#!/usr/bin/env python3
"""Bounded staged adapter/bridge test; CPU-only GL stubs, no real renderer."""
import hashlib
import json
import os
from pathlib import Path
import resource
import shlex
import shutil
import subprocess
import tempfile

script = Path(__file__).resolve()
staged = script.parents[2]
repo = next(p for p in script.parents
            if (p / ".git").exists() and (p / "pc_port/src/hd2d/hd2d_policy.c").is_file())
os.chdir(repo)
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
if "HD2D_SCENE_TEST_OUT" in os.environ:
    out = Path(os.environ["HD2D_SCENE_TEST_OUT"]).resolve()
    out.mkdir(parents=True, exist_ok=True)
    assert not any(out.iterdir()), f"Refusing to overwrite evidence: {out}"
else:
    out = Path(tempfile.mkdtemp(prefix="scene-buffers-test-", dir=staged.parent))
print(f"OUTPUT {out}", flush=True)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


sources = [
    staged / "pc_port/src/hd2d/hd2d_bridge.c",
    staged / "pc_port/src/hd2d/hd2d_scene_buffers_adapter.c",
    staged / "pc_port/tests/hd2d/hd2d_scene_buffers_test.c",
    repo / "pc_port/src/hd2d/hd2d_policy.c",
]
inputs = sources + [
    script, staged / "pc_port/include_shim/hd2d_scene_buffers.h",
    repo / "pc_port/include/xg_plat/hd2d.h",
    repo / "pc_port/include_shim/hd2d_integration.h",
    repo / "pc_port/include_shim/hd2d_sprite.h",
    repo / "pc_port/src/port_dev_menu.h",
    repo / "pc_port/src/quick_checkpoint.h",
]
pins = {str(p.relative_to(repo)): sha(p) for p in inputs}
cc = shlex.split(os.environ.get("CC", "cc"))
assert cc and shutil.which(cc[0]), "Requested compiler is unavailable"
compiler = Path(shutil.which(cc[0])).resolve()
toolchain = {"command": cc, "path": str(compiler), "sha256": sha(compiler),
             "version": subprocess.check_output(cc + ["--version"], text=True)}
modes = os.environ.get("MODES", "O0 O2 UBSan").split()
assert modes and all(m in ("O0", "O2", "UBSan") for m in modes)
env = os.environ.copy()
env.pop("XENO_HD2D_OCTOPATH", None)
env.pop("XENO_HD2D_OPENING_MAPS", None)
env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=0"
commands, returncodes = [], {}


def save(status):
    artifact_pins = {str(p): sha(p) for p in out.iterdir()
                     if p.is_file() and p.name != "receipt.json"}
    (out / "receipt.json").write_text(json.dumps({
        "status": status, "source_pins": pins, "toolchain": toolchain,
        "commands": commands, "returncodes": returncodes,
        "artifact_pins": artifact_pins,
        "scope": "Actual staged bridge/adapter and production policy, synthetic "
                 "field state, GL viewport/present stubs, optional menu API. "
                 "No real GL texture, provenance registry, renderer integration, "
                 "capture, visual, gameplay or device acceptance.",
    }, indent=2) + "\n")


def run(command, log):
    commands.append(command)
    with (out / log).open("w") as stream:
        result = subprocess.run(command, env=env, stdout=stream,
                                stderr=subprocess.STDOUT, timeout=120)
    returncodes[log] = result.returncode
    if result.returncode:
        save("FAIL: " + log)
        print((out / log).read_text(), flush=True)
        raise SystemExit(1)


save("PREPARING")
for api in ("present", "absent"):
    for mode in modes:
        flags = (["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]
                 if mode == "UBSan" else ["-" + mode])
        name = f"{api}-{mode}"
        binary = out / name
        command = cc + ["-std=c11", "-Wall", "-Wextra", "-Werror"] + flags
        command += shlex.split(os.environ.get("CPPFLAGS", ""))
        command += shlex.split(os.environ.get("CFLAGS", ""))
        command += ["-UNDEBUG", "-I" + str(staged / "pc_port/include_shim"),
                    "-Ipc_port/include_shim", "-Ipc_port/include", "-Ipc_port/src"]
        if api == "absent":
            command += ["-DHD2D_TEST_NO_VALUE_API"]
        command += [str(p) for p in sources] + ["-o", str(binary)]
        run(command, name + ".compile.log")
        run([str(binary)], name + ".run.log")
        assert "HD2D SCENE BUFFERS PASS" in (out / (name + ".run.log")).read_text()
        print(name + " PASS", flush=True)
        save("MODES IN PROGRESS")
for path, digest in pins.items():
    assert sha(repo / path) == digest, f"Input changed during test: {path}"
save("PASS")
print("HD2D SCENE BUFFERS SUITE PASS", flush=True)
