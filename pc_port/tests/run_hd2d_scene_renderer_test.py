#!/usr/bin/env python3
"""Real SDL offscreen/Mesa fixture for the staged coverage renderer.

No downloads or game assets. CC/CXX/CPPFLAGS/CFLAGS/CXXFLAGS/LDFLAGS and
SDL_CFLAGS/SDL_LIBS/GL_CFLAGS/GL_LIBS accept shell-style argument lists.
MODES defaults to O0 O2 UBSan. HD2D_SCENE_RENDERER_OUT must name a new evidence
directory when provided. Root owns CPU0/nice19 and every compile/test run.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def args(name: str, default: str = "") -> list[str]:
    return shlex.split(os.environ.get(name, default))


def dependency(name: str, package: str, kind: str) -> list[str]:
    if name in os.environ:
        return args(name)
    return shlex.split(subprocess.check_output(
        ["pkg-config", kind, package], text=True))


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    tree = Path(__file__).resolve().parents[2]
    repo = next(parent for parent in (tree, *tree.parents)
                if (parent / "include/common.h").is_file()
                and (parent / "pc_port/src/guest_prim_link.c").is_file())
    configured_out = os.environ.get("HD2D_SCENE_RENDERER_OUT")
    if configured_out:
        out = Path(configured_out).resolve()
        out.mkdir(parents=True, exist_ok=False)
    else:
        out = Path(tempfile.mkdtemp(prefix="xeno-hd2d-scene-renderer-"))
    fixture = tree / "pc_port/tests/hd2d_scene_renderer_test.cpp"
    adapter = tree / "pc_port/src/hd2d/hd2d_scene_buffers_adapter.c"
    renderer = tree / "pc_port/src/psycross_hd2d_scene.inl"
    pins = [fixture, adapter, renderer,
            tree / "pc_port/include_shim/hd2d_scene_buffers.h",
            tree / "pc_port/include_shim/hd2d_world_packets.h",
            repo / "pc_port/include_shim/hd2d_integration.h",
            repo / "pc_port/include/xg_plat/hd2d.h",
            Path(__file__).resolve()]
    receipt = {
        "status": "RUNNING",
        "scope": "Real included renderer and one-shot adapter; synthetic geometry/"
                 "provenance/bridge; no game/CLUT/fixed-UV proof",
        "sources": {str(path): sha(path) for path in pins},
        "modes": [],
    }
    receipt_path = out / "receipt.json"

    def save() -> None:
        receipt_path.write_text(json.dumps(receipt, indent=2) + "\n")

    save()
    try:
        modes = args("MODES", "O0 O2 UBSan")
        regimes = {
            "O0": ["-O0", "-g"],
            "O2": ["-O2", "-g"],
            "UBSan": ["-O2", "-g", "-fsanitize=undefined",
                      "-fno-sanitize-recover=all"],
        }
        if not modes or any(mode not in regimes for mode in modes):
            raise ValueError("MODES must select O0, O2 and/or UBSan")
        includes = ["-I" + str(path) for path in [
            tree / "pc_port/include_shim",
            repo / "pc_port/include_shim",
            repo / "pc_port/include",
        ]]
        sdl_flags = dependency("SDL_CFLAGS", "sdl2", "--cflags")
        sdl_libs = dependency("SDL_LIBS", "sdl2", "--libs")
        gl_flags = dependency("GL_CFLAGS", "gl", "--cflags")
        gl_libs = dependency("GL_LIBS", "gl", "--libs")
        common = args("CPPFLAGS") + includes + ["-UNDEBUG", "-Wall", "-Wextra", "-Werror"]
        run_env = dict(os.environ)
        run_env.setdefault("SDL_VIDEODRIVER", "offscreen")
        run_env.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
        receipt["runtime_environment"] = {
            key: run_env[key] for key in ("SDL_VIDEODRIVER", "LIBGL_ALWAYS_SOFTWARE")
        }
        for mode in modes:
            binary = out / ("scene-renderer-" + mode)
            adapter_object = out / ("adapter-" + mode + ".o")
            commands = [
                args("CC", "cc") + args("CFLAGS") + common + regimes[mode]
                + ["-std=c11", "-c", str(adapter), "-o", str(adapter_object)],
                args("CXX", "c++") + args("CXXFLAGS") + common + regimes[mode]
                + sdl_flags + gl_flags + ["-std=c++11", str(fixture), str(adapter_object)]
                + args("LDFLAGS") + sdl_libs + gl_libs + ["-o", str(binary)],
            ]
            result = {"mode": mode, "compile_commands": commands}
            receipt["modes"].append(result)
            save()
            with (out / (mode + ".compile.log")).open("w") as log:
                for command in commands:
                    subprocess.run(command, cwd=repo, stdout=log,
                                   stderr=subprocess.STDOUT, check=True)
            result["binary_sha256"] = sha(binary)
            with (out / (mode + ".stdout")).open("w") as stdout, (
                out / (mode + ".stderr")
            ).open("w") as stderr:
                subprocess.run([str(binary)], cwd=repo, env=run_env,
                               stdout=stdout, stderr=stderr, check=True)
            if "HD2D scene renderer PASS checks=" not in (out / (mode + ".stdout")).read_text():
                raise RuntimeError("Missing real-GL fixture PASS receipt")
            result["status"] = "PASS"
            save()
        receipt["status"] = "PASS"
    except BaseException as error:
        receipt["status"] = "FAIL"
        receipt["error"] = str(error)
        raise
    finally:
        save()
        print(receipt_path)


if __name__ == "__main__":
    main()
