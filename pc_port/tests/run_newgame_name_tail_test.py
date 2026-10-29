#!/usr/bin/env python3
"""Compile actual initializer/decoder TUs; retain private-oracle test receipts."""
import hashlib
import json
import os
from pathlib import Path
import resource
import shlex
import shutil
import subprocess
import tempfile


repo = Path(__file__).resolve().parents[2]
os.chdir(repo)
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
if "NEWGAME_NAME_TAIL_TEST_OUT" in os.environ:
    out = Path(os.environ["NEWGAME_NAME_TAIL_TEST_OUT"]).resolve()
    out.mkdir(parents=True, exist_ok=True)
    assert not any(out.iterdir()), f"Refusing to overwrite test evidence: {out}"
else:
    out = Path(tempfile.mkdtemp(prefix="newgame-name-tail-", dir=repo / "scratchpad"))
print(f"OUTPUT {out}", flush=True)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


sources = [
    "src/slus_006.64/system/temp3.c",
    "src/slus_006.64/system/system.c",
    "pc_port/tests/newgame_name_tail_test.c",
    "pc_port/tests/run_newgame_name_tail_test.py",
    "pc_port/src/battle_mips_adapter.c",
    "pc_port/src/battle_mips_adapter.h",
    "include/types.h",
    "include/main/game.h",
    "include/system/sound.h",
    "asm/slus_006.64/matchings/system/temp3c/func_8001B970.s",
    "asm/slus_006.64/nonmatchings/system/system/func_80033B34.s",
]
pins = {p: sha(p) for p in sources}
image = Path("disc/SLUS_006.64").read_bytes()
bodies = {
    "initializer": (0xC170, 412, "5d425a714e0f54b81fac1e0330e11eb2b0cb0a08c12cba9506e0191edb773a95"),
    "decoder": (0x24334, 120, "62044b5a4441be3669dcf72b4e7e6968a44658d9fb50ee09c74add1e273f670c"),
}
for name, (offset, size, digest) in bodies.items():
    assert hashlib.sha256(image[offset:offset + size]).hexdigest() == digest, name
private = {"disc/SLUS_006.64": hashlib.sha256(image).hexdigest()}
del image

env = os.environ.copy()
env["XENO_CPU_EXECUTION_STATS"] = "1"
env["XENO_DISABLE_CPU_INTERPRETER"] = "0"
env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=0"
modes = os.environ.get("NEWGAME_NAME_TAIL_TEST_MODES", "O0 O2 UBSan").split()
assert modes and all(m in ("O0", "O2", "UBSan") for m in modes)
cc = shlex.split(os.environ.get("CC", "gcc"))
assert cc, "CC must name a compiler"
executable = shutil.which(cc[0])
assert executable, f"Missing compiler: {cc[0]}"
executable = Path(executable).resolve()
toolchain = {
    "command": cc,
    "executable": str(executable),
    "sha256": sha(executable),
    "version": subprocess.check_output(cc + ["--version"], text=True),
}
# Match the existing production-TU harness: permissive legacy declarations
# need different flags under Clang. Keep the new fixture/adapter warnings strict.
legacy_warnings = (["-Wno-everything"] if "clang" in toolchain["version"].lower()
                   else ["-fpermissive", "-w"])
initializer_compat = []
compatibility_header = None
if "clang" in toolchain["version"].lower():
    # Clang diagnoses these earlier implicit-int calls when their later void
    # definitions are parsed, even though their sections are discarded. Declare
    # the exact existing signatures in the harness, without rewriting the TU.
    declarations = [
        "void GamePartyStreamLoadSkinData(int arg0);",
        "void GamePartySyncStreamedData(void);",
        "void func_8001B53C(int index);",
        "void func_8001B844(void);",
    ]
    production = Path(sources[0]).read_text()
    for declaration in declarations:
        assert declaration[:-1] + " {" in production, declaration
    header = out / "temp3-compatibility.h"
    header.write_text("/* Test-only declarations; signatures from actual temp3.c. */\n" +
                      "\n".join(declarations) + "\n")
    initializer_compat = ["-include", str(header)]
    compatibility_header = {"path": str(header), "sha256": sha(header),
                            "declarations": declarations,
                            "scope": "Clang initializer TU and its negative-control copies only"}
common = [
    "-std=gnu17", "-fno-pie", "-fno-builtin", "-ffunction-sections", "-fdata-sections",
    "-DXENO_PC_PORT", "-DSKIP_ASM", "-D_LANGUAGE_C", "-DUSE_EXTENDED_PRIM_POINTERS=0",
    "-include", "assert.h",
    "-Ipc_port/include_shim", "-Iinclude", "-Ipc_port/src",
    "-Ipc_port/extern/PsyCross/include", "-Ipc_port/extern/PsyCross/include/psx",
    # Copied mutants retain the original quoted relative include unchanged.
    "-I" + str(repo / "src/slus_006.64/system"),
]
common += shlex.split(os.environ.get("CPPFLAGS", ""))
common += shlex.split(os.environ.get("CFLAGS", ""))
common += ["-UNDEBUG"]
commands, returncodes, negatives = [], {}, {}
scope = (
    "Actual complete initializer and decoder translation units with synthetic "
    "archive/heap boundaries; 31 full records and all other GameState bytes, "
    "repeatability, within-call tail reuse, volume/mode effects; complete private "
    "412+120-byte original oracle under explicit zero and nonzero initial-tail "
    "histories. Native legs disable interpretation. Deliberate zero initial tail "
    "does not claim arbitrary retail stack history, fault/CPU-state equivalence, "
    "full-game or owner-device acceptance."
)


def save(status):
    artifacts = {
        str(p.relative_to(repo)): sha(p)
        for p in out.rglob("*") if p.is_file() and p.name != "receipt.json"
    }
    (out / "receipt.json").write_text(json.dumps({
        "status": status, "source_pins": pins, "private_inputs": private,
        "private_bodies": {k: {"offset": v[0], "bytes": v[1], "sha256": v[2]}
                           for k, v in bodies.items()},
        "toolchain": toolchain, "commands": commands, "returncodes": returncodes,
        "compatibility_header": compatibility_header,
        "negative_controls": negatives, "artifact_pins": artifacts, "scope": scope,
        "overrides": {k: os.environ[k] for k in
                      ("CC", "CPPFLAGS", "CFLAGS", "NEWGAME_NAME_TAIL_TEST_MODES")
                      if k in os.environ},
    }, indent=2) + "\n")


def run(command, log, expected=0):
    commands.append(command)
    with (out / log).open("w") as stream:
        result = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT,
                                env=env, timeout=120)
    if expected is not None and result.returncode != expected:
        save("FAIL: " + log)
        print((out / log).read_text(), flush=True)
        raise SystemExit(result.returncode if result.returncode > 0 else 1)
    return result.returncode


save("PREPARING")
# Probe the requested sanitizer before any positive mode passes. Pattern init
# makes removal of the native initializer fail deterministically, without
# assuming a particular host stack reuse layout.
probe = out / "probe.c"
probe.write_text("int main(void) { return 0; }\n")
probe_flags = ["-ftrivial-auto-var-init=pattern"]
if "UBSan" in modes:
    probe_flags += ["-fsanitize=undefined", "-fno-sanitize-recover=all"]
run(cc + probe_flags + [str(probe), "-o", str(out / "probe")], "probe-compile.log")
run([str(out / "probe")], "probe-run.log")

mode_objects = {}
for mode in modes:
    flags = (["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]
             if mode == "UBSan" else ["-" + mode])
    objects = []
    for label, source in (
        ("initializer", sources[0]), ("decoder", sources[1]),
        ("test", sources[2]), ("cpu", "pc_port/src/battle_mips_adapter.c"),
    ):
        obj = out / f"{mode}.{label}.o"
        warnings = legacy_warnings if label in ("initializer", "decoder") \
            else ["-Wall", "-Wextra", "-Werror"]
        extra = (initializer_compat + ["-ftrivial-auto-var-init=pattern"]
                 if label == "initializer" else [])
        run(cc + common + flags + warnings + extra + ["-c", source, "-o", str(obj)],
            f"{mode}.{label}-compile.log")
        objects.append(obj)
    binary = out / f"{mode}.test"
    run(cc + ["-no-pie"] + flags + ["-Wl,--gc-sections"] +
        [str(p) for p in objects] + ["-o", str(binary)], f"{mode}.link.log")
    rc = run([str(binary)], f"{mode}.log", expected=None)
    returncodes[mode] = rc
    if rc != 0 or "NAME TAIL PASS" not in (out / f"{mode}.log").read_text():
        save("FAIL: " + mode)
        print((out / f"{mode}.log").read_text(), flush=True)
        raise SystemExit(1)
    mode_objects[mode] = (flags, objects)
    print(f"{mode} PASS", flush=True)
    save("POSITIVE MODES IN PROGRESS")

# Keep negative controls to the two semantic risks of this small repair.
# Each must compile/link, then fail the full-state output assertion.
base = Path(sources[0]).read_text()
assert base.count("u8 tmp[48] = {0};") == 1
assert base.count("        func_80033B34((u16*)tmp, tmp + 24, k / 2);") == 1
mutants = {
    "missing-initialization": base.replace("u8 tmp[48] = {0};", "u8 tmp[48];", 1),
    "cleared-between-records": base.replace(
        "        func_80033B34((u16*)tmp, tmp + 24, k / 2);",
        "        for (s32 tail = 24; tail < 48; ++tail) tmp[tail] = 0;\n"
        "        func_80033B34((u16*)tmp, tmp + 24, k / 2);", 1),
}
mode = "O2" if "O2" in mode_objects else modes[0]
flags, objects = mode_objects[mode]
for name, source in mutants.items():
    path = out / f"{name}.c"
    path.write_text(source)
    obj = out / f"{name}.o"
    binary = out / f"{name}.test"
    run(cc + common + flags + legacy_warnings + initializer_compat +
        ["-ftrivial-auto-var-init=pattern",
        "-c", str(path), "-o", str(obj)], f"{name}.compile.log")
    run(cc + ["-no-pie"] + flags + ["-Wl,--gc-sections", str(obj)] +
        [str(p) for p in objects[1:]] + ["-o", str(binary)], f"{name}.link.log")
    rc = run([str(binary)], f"{name}.log", expected=None)
    rejected = rc != 0 and "NAME TAIL FAIL native GameState" in (out / f"{name}.log").read_text()
    negatives[name] = {"returncode": rc, "expected_output_failure": rejected,
                       "source_sha256": sha(path)}
    if not rejected:
        save("FAIL: negative control " + name)
        raise SystemExit(f"Negative control not rejected by expected assertion: {name}")
    print(f"REJECTED {name}", flush=True)

for path, digest in pins.items():
    assert sha(path) == digest, f"Source changed during tests: {path}"
save("PASS")
print("NEWGAME NAME TAIL SUITE PASS", flush=True)
