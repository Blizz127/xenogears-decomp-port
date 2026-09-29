#!/usr/bin/env python3
"""Retail-instruction differential for the installed func_80032F54 body.

The test compares the complete 0xA0000-byte scratch arena. Heap policy remains a
bounded spy: allocation requests, tags, flags, order, and returned addresses are
checked, while allocator metadata, fragmentation, and failure policy are outside
this regression. Oversized row/raster requests stop at that explicit boundary.
No renderer, VRAM upload, game process, or desktop input is exercised.
"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
TEST_SOURCE = Path(__file__).resolve().with_name(
    "battle_window_constructor_retail_test.c"
)
SYSTEM_SOURCE = ROOT / "src/slus_006.64/system/system.c"
GPU_SOURCE = ROOT / "pc_port/extern/PsyCross/src/psx/LIBGPU.C"
DISC_EXE = ROOT / "disc/SLUS_006.64"
CLANG = Path("/home/linuxbrew/.linuxbrew/bin/clang")

DISC_SHA256 = "dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
SOURCE_PINS = {
    "pc_port/extern/PsyCross/src/psx/LIBGPU.C":
        "de27cc432715bec2da527f06f439d900818080051685b1c0a2c37666d5f427ca",
    "pc_port/extern/PsyCross/include/psx/libgpu.h":
        "97dc22c5d2830eed91cf0ca74f608d67d4cc80c7d620b2dffbb963772214626b",
    "pc_port/src/battle_mips_adapter.c":
        "4e0e896434e6d2f5452b8ac6f82799414ce8378bf9278fa7403cd083a4f2bf22",
    "pc_port/src/battle_mips_adapter.h":
        "acd0133574e1cb546ce2b33ee582d7b8dff48c988298c6ac4fa6cb65e4b5f12a",
}
RETAIL_RANGES = {
    "constructor": (0x23754, 0x4D8,
        "e48e0f7d63d082ab40246cb43b6bf7e00bd53775a72e822c744f0d10ed0f6be5"),
    "heapalloc": (0x223DC, 0x394,
        "769fc2a84de3a5c0fc3829121bd31f8821ce5dad2cfa1f5dcfe6fcbbab14f6f0"),
    "contenttag": (0x22CB8, 0x0C,
        "1b90c03fa1e2fc230871ff31267d0327e2f846ed336ee8ce2e9a05f12dd05af4"),
    "gettpage": (0x3421C, 0x3C,
        "ca97bb54edac0e68e757a06d6a25eb10c8fea745f5234450b15d0997d759bf42"),
    "semitrans": (0x343FC, 0x28,
        "f7bd3db94e23b108242338e61ab13d8b1f6877d7a227b90ed3f11c550612804b"),
    "setdrawmode": (0x35CDC, 0x58,
        "7b4adc8244e499de946aa604de368434cb2e7aef9ac7bffdd3c9d79c30edb0c5"),
    "getmode": (0x361DC, 0x58,
        "165c0f29e23a39119c72207a8697b899ee1174ecab5da0dac1ab671c922b7d99"),
    "gettw": (0x36410, 0x84,
        "9762c42142ce9b5872675cd3f067fa1c6d07ef10e3880fcac24044f89497f9f6"),
}
LOADED_RETAIL = ("constructor", "gettpage", "semitrans", "setdrawmode", "getmode", "gettw")
PREFIX = """#include <stdint.h>
#include <string.h>
#include "common.h"
#include "psyq/libgpu.h"
extern void HeapSetCurrentContentType(u_short);
extern void *HeapAlloc(u_int, u_int);
extern s16 g_SystemPalette1, g_SystemPalette2;
"""
COMMON = [
    "-std=gnu17", "-fno-pie", "-fno-builtin", "-DXENO_PC_PORT", "-DSKIP_ASM",
    "-D_LANGUAGE_C", "-DUSE_EXTENDED_PRIM_POINTERS=0", "-ffunction-sections",
    "-fdata-sections", "-Ipc_port/include_shim", "-Iinclude", "-Ipc_port/src",
    "-Ipc_port/extern/PsyCross/include",
    "-Ipc_port/extern/PsyCross/include/psx", "-include", "assert.h",
    "-Wall", "-Wextra", "-Werror",
]


class TestFailure(RuntimeError):
    pass


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_pinned(path: Path, expected: str) -> bytes:
    try:
        data = path.read_bytes()
    except OSError as exc:
        raise TestFailure(f"missing pinned input {path}: {exc}") from exc
    actual = sha256(data)
    if actual != expected:
        raise TestFailure(f"pin mismatch {path}: expected {expected}, got {actual}")
    return data


def extract_body(source: str) -> str:
    start_marker = "void func_80032F54("
    end_marker = "\nunsigned int ResolveArchiveEntryPointers"
    if source.count(start_marker) != 1:
        raise TestFailure(f"constructor start marker count={source.count(start_marker)}")
    start = source.index(start_marker)
    if source.count(end_marker, start) != 1:
        raise TestFailure(f"constructor end marker count={source.count(end_marker, start)}")
    return source[start:source.index(end_marker, start)]


def replace_once(source: str, old: str, new: str, name: str) -> str:
    count = source.count(old)
    if count != 1:
        raise TestFailure(f"semantic control {name} anchor count={count}")
    mutated = source.replace(old, new, 1)
    if mutated == source:
        raise TestFailure(f"semantic control {name} made no change")
    return mutated


def generate_controls(body: str) -> dict[str, str]:
    raster_clear = (
        "    memset((void*)(uintptr_t)*(u32*)(pWindow + 0x2C), 0, "
        "*(s16*)(pWindow + 0x12) * 0x1C);\n"
    )
    specs = {
        "raster_clear": (
            "    *(u8*)(pWindow + 0x4B) = 3;",
            raster_clear + "    *(u8*)(pWindow + 0x4B) = 3;"),
        "palette_swap": (
            "oddRow ? g_SystemPalette2 : g_SystemPalette1",
            "oddRow ? g_SystemPalette1 : g_SystemPalette2"),
        "right_width_256": (
            "*(s16*)(pWindow + 0x08) - 0xF0",
            "*(s16*)(pWindow + 0x08) - 0x100"),
        "omit_second_allocation": (
            "HeapAlloc(*(s16*)(pWindow + 0x12) * 0x1C, 2)",
            "(void*)0"),
        "masked_x": ("| (u32)x;", "| ((u32)x & 0xFFFFu);"),
        "raw_height": ("    height = (s16)height;\n", ""),
        "unsigned_left_width": (
            "(0x000D0000u | (u32)(s32)*(s16*)(pWindow + 0x08))",
            "(0x000D0000u | *(u16*)(pWindow + 0x08))"),
    }
    return {name: replace_once(body, old, new, name)
            for name, (old, new) in specs.items()}


def extract_gpu_bodies(source: str) -> str:
    bodies = []
    for name in ("GetTPage", "SetSemiTrans", "SetDrawMode"):
        matches = re.findall(
            rf"^(?:u_short|void) {name}\([^\n]+\)\n\{{.*?^\}}",
            source, re.MULTILINE | re.DOTALL)
        if len(matches) != 1:
            raise TestFailure(f"PsyCross {name} body count={len(matches)}")
        bodies.append(matches[0].replace(name + "(", "GpuBody_" + name + "(", 1))
    return '#include "common.h"\n#include "psyq/libgpu.h"\n' + "\n\n".join(bodies) + "\n"


def run_command(command: list[str], *, timeout: int = 60) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=timeout)
    if proc.returncode != 0:
        output = (proc.stdout + proc.stderr)[-4000:]
        raise TestFailure(f"command failed rc={proc.returncode}: {' '.join(command)}\n{output}")
    return proc


def compile_variant(
    out: Path, compiler: str, flags: list[str], label: str, body_path: Path,
    shared: dict[str, Path],
) -> Path:
    obj = out / f"{label}.body.o"
    run_command([compiler, *COMMON, *flags, "-c", str(body_path), "-o", str(obj)])
    exe = out / f"{label}.test"
    run_command([
        compiler, "-no-pie", *flags, "-Wl,--gc-sections", str(obj),
        str(shared["fixture"]), str(shared["gpu"]), str(shared["cpu"]),
        "-o", str(exe),
    ])
    return exe


def run_fixture(exe: Path, out: Path, label: str, expect_pass: bool) -> dict[str, object]:
    proc = subprocess.run([str(exe), str(out)], cwd=ROOT, text=True,
                          capture_output=True, timeout=30)
    log = proc.stdout + proc.stderr
    (out / f"{label}.log").write_text(log)
    summary = [line for line in log.splitlines()
               if line.startswith("CONSTRUCTOR_RETAIL_")]
    sanitizer = [line for line in log.splitlines() if "runtime error:" in line]
    prefix = "CONSTRUCTOR_RETAIL_PASS" if expect_pass else "CONSTRUCTOR_RETAIL_FAIL"
    expected_rc = 0 if expect_pass else 1
    if proc.returncode != expected_rc or len(summary) != 1 or not summary[0].startswith(prefix) or sanitizer:
        raise TestFailure(
            f"{label} expected {prefix}/rc={expected_rc}, got rc={proc.returncode}, "
            f"summaries={summary}, sanitizer={sanitizer}\n{log[-4000:]}")
    print(f"{label}: {summary[0]}")
    return {
        "label": label,
        "returncode": proc.returncode,
        "summary": summary[0],
        "log_sha256": sha256(log.encode()),
        "executable_sha256": sha256(exe.read_bytes()),
    }


def compiler_version(compiler: str) -> str:
    return run_command([compiler, "--version"]).stdout.splitlines()[0]


def main() -> int:
    if len(sys.argv) != 1:
        raise TestFailure("this regression accepts no arguments")
    gcc = shutil.which("gcc")
    if gcc is None:
        raise TestFailure("gcc not found")
    if not CLANG.is_file() or not os.access(CLANG, os.X_OK):
        raise TestFailure(f"required Clang not executable: {CLANG}")

    out = Path(tempfile.mkdtemp(prefix="xeno-window-constructor-retail-"))
    print(f"BATTLE_WINDOW_CONSTRUCTOR_RETAIL_OUTPUT {out}", flush=True)

    exe = read_pinned(DISC_EXE, DISC_SHA256)
    input_hashes: dict[str, str] = {"disc/SLUS_006.64": sha256(exe)}
    slices: dict[str, bytes] = {}
    for name, (offset, size, expected) in RETAIL_RANGES.items():
        data = exe[offset:offset + size]
        actual = sha256(data)
        if len(data) != size or actual != expected:
            raise TestFailure(
                f"retail slice {name} mismatch: size={len(data)}/{size} sha={actual}/{expected}")
        slices[name] = data
        input_hashes[f"retail.{name}"] = actual

    for relative, expected in SOURCE_PINS.items():
        input_hashes[relative] = sha256(read_pinned(ROOT / relative, expected))
    system_text = SYSTEM_SOURCE.read_text()
    body = extract_body(system_text)
    controls = generate_controls(body)
    gpu_text = GPU_SOURCE.read_text()
    gpu_body = extract_gpu_bodies(gpu_text)
    input_hashes[str(SYSTEM_SOURCE.relative_to(ROOT))] = sha256(system_text.encode())
    input_hashes["func_80032F54.body"] = sha256(body.encode())
    input_hashes[str(TEST_SOURCE.relative_to(ROOT))] = sha256(TEST_SOURCE.read_bytes())

    results: list[dict[str, object]] = []
    for name in LOADED_RETAIL:
        (out / f"retail.{name}.bin").write_bytes(slices[name])
    production = out / "constructor.production.c"
    production.write_text(PREFIX + body)
    for name, mutant in controls.items():
        (out / f"constructor.control-{name}.c").write_text(PREFIX + mutant)
    gpu_path = out / "gpu.actual.c"
    gpu_path.write_text(gpu_body)

    modes = [
        ("O0", gcc, ["-O0"]),
        ("O2", gcc, ["-O2"]),
        ("UBSan", str(CLANG), ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
    ]
    shared_by_mode: dict[str, dict[str, Path]] = {}
    for mode, compiler, flags in modes:
        shared: dict[str, Path] = {}
        for unit, path in (
            ("fixture", TEST_SOURCE),
            ("gpu", gpu_path),
            ("cpu", ROOT / "pc_port/src/battle_mips_adapter.c"),
        ):
            obj = out / f"{mode}.{unit}.o"
            run_command([compiler, *COMMON, *flags, "-c", str(path), "-o", str(obj)])
            shared[unit] = obj
        shared_by_mode[mode] = shared
        built = compile_variant(out, compiler, flags, f"{mode}.production",
                                production, shared)
        results.append(run_fixture(built, out, f"{mode}.production", True))

    mode = "O2"
    compiler = gcc
    flags = ["-O2"]
    for name in sorted(controls):
        body_path = out / f"constructor.control-{name}.c"
        built = compile_variant(out, compiler, flags, f"O2.control-{name}",
                                body_path, shared_by_mode[mode])
        results.append(run_fixture(built, out, f"O2.control-{name}", False))

    manifest = {
        "status": "PASS",
        "cases_per_production_build": 124,
        "allocator_boundary_only_per_build": 12,
        "production_builds": 3,
        "semantic_controls": len(controls),
        "compilers": {
            "gcc": compiler_version(gcc),
            "clang_ubsan": compiler_version(str(CLANG)),
        },
        "inputs_sha256": input_hashes,
        "results": results,
    }
    manifest_text = json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    (out / "manifest.json").write_text(manifest_text)
    print("CONSTRUCTOR_RETAIL_MANIFEST " +
          json.dumps(manifest, sort_keys=True, separators=(",", ":")))
    print("BATTLE_WINDOW_CONSTRUCTOR_RETAIL_TEST_PASS "
          "production=O0,O2,UBSan cases_each=124 semantic_controls=7 "
          f"manifest_sha256={sha256(manifest_text.encode())}")
    print(f"BATTLE_WINDOW_CONSTRUCTOR_RETAIL_OUTPUT_RETAINED {out}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (TestFailure, OSError, subprocess.TimeoutExpired) as exc:
        print(f"BATTLE_WINDOW_CONSTRUCTOR_RETAIL_TEST_FAIL {exc}", file=sys.stderr)
        raise SystemExit(1)
