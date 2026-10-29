#!/usr/bin/env python3
"""Compile/run actual CPU provenance and domain-aware link sources.

Works from this staged tree or after root lands the files. No downloads.
CC/CPPFLAGS/CFLAGS/LDFLAGS accept shell-style argument lists. Select regimes
with MODES="O0 O2 UBSan"; set HD2D_WORLD_PACKETS_OUT to a new evidence directory.
The invoking integrator owns CPU affinity, priority and the sole heavy slot.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    tree = Path(__file__).resolve().parents[2]
    repo = next(
        parent for parent in (tree, *tree.parents)
        if (parent / "pc_port/src/guest_prim_link.c").is_file()
        and (parent / "include/common.h").is_file()
    )
    configured_out = os.environ.get("HD2D_WORLD_PACKETS_OUT")
    if configured_out:
        out = Path(configured_out).resolve()
        out.mkdir(parents=True, exist_ok=False)
    else:
        out = Path(tempfile.mkdtemp(prefix="xeno-hd2d-world-"))
    sources = [
        tree / "pc_port/tests/hd2d_world_packets_test.c",
        tree / "pc_port/src/hd2d/hd2d_world_packets.c",
        tree / "pc_port/src/model_prim_link.c",
        repo / "pc_port/src/guest_prim_link.c",
    ]
    headers = [
        tree / "pc_port/include_shim/hd2d_world_packets.h",
        repo / "pc_port/include_shim/guest_prim_link.h",
        repo / "pc_port/src/model_prim_link.h",
        repo / "pc_port/include_shim/psx_memory.h",
    ]
    modes = shlex.split(os.environ.get("MODES", "O0 O2 UBSan"))
    regimes = {
        "O0": ["-O0", "-g"],
        "O2": ["-O2", "-g"],
        "UBSan": ["-O2", "-g", "-fsanitize=undefined",
                  "-fno-sanitize-recover=all"],
    }
    if not modes or any(mode not in regimes for mode in modes):
        raise SystemExit("MODES must select O0, O2 and/or UBSan")
    base = (
        shlex.split(os.environ.get("CC", "cc"))
        + shlex.split(os.environ.get("CPPFLAGS", ""))
        + shlex.split(os.environ.get("CFLAGS", ""))
        + ["-std=gnu17", "-UNDEBUG", "-DXENO_PC_PORT",
           "-DINCLUDE_ASM_USE_MACRO_INC=0", "-D_LANGUAGE_C",
           "-DUSE_EXTENDED_PRIM_POINTERS=0", "-fno-pie",
           "-Wall", "-Wextra", "-Werror", "-Wconversion",
           "-Wsign-conversion", "-Wshadow", "-Wundef"]
        + ["-I" + str(path) for path in [
            tree / "pc_port/include_shim",
            repo / "pc_port/include_shim",
            repo / "pc_port/extern/PsyCross/include",
            repo / "pc_port/extern/PsyCross/include/psx",
            repo / "include", repo / "pc_port/src",
        ]]
    )
    results = {
        "scope": "CPU helper plus actual model/domain link; no production GL proof",
        "sources": {str(path): sha(path) for path in sources + headers
                    + [Path(__file__).resolve()]},
        "modes": [],
        "status": "RUNNING",
    }
    receipt = out / "receipt.json"

    def save() -> None:
        receipt.write_text(json.dumps(results, indent=2) + "\n")

    save()
    try:
        for mode in modes:
            binary = out / ("world-packets-" + mode)
            command = base + regimes[mode] + [str(path) for path in sources]
            command += shlex.split(os.environ.get("LDFLAGS", ""))
            command += ["-Wl,--wrap=calloc,--wrap=free", "-no-pie", "-o", str(binary)]
            result = {"mode": mode, "compile_command": command}
            results["modes"].append(result)
            save()
            with (out / (mode + ".compile.log")).open("w") as log:
                subprocess.run(command, cwd=repo, stdout=log,
                               stderr=subprocess.STDOUT, check=True)
            result["binary_sha256"] = sha(binary)
            with (out / (mode + ".stdout")).open("w") as stdout, (
                out / (mode + ".stderr")
            ).open("w") as stderr:
                subprocess.run([str(binary)], cwd=repo, stdout=stdout,
                               stderr=stderr, check=True)
            if not (out / (mode + ".stdout")).read_text().startswith(
                "HD2D world provenance PASS checks="
            ):
                raise RuntimeError("Missing fixture PASS receipt")
            result["status"] = "PASS"
            save()
        results["status"] = "PASS"
    except BaseException as error:
        results["status"] = "FAIL"
        results["error"] = str(error)
        raise
    finally:
        save()
        print(receipt)


if __name__ == "__main__":
    main()
