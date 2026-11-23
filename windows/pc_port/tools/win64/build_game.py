#!/usr/bin/env python3
"""Compile the r7.21 hybrid Windows game from its verified, generated release inputs."""

import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TOOL_DIR = Path(__file__).resolve().parent
VERIFIED = {
    "xeno-port.link-args": "dd36fd29499ab8ed989399fb9e82453665799d8147f8d4481c7e6ce78a7b3b10",
    "matching-before.nm": "736126858aa12f13502a9bdbdd7e429b0b239cf7d51c79980fc155819dbf3507",
    "matching-after.nm": "1552eb922f01e5682e7af26492e01a0e36c41a516c7735548ef20b09b59b9332",
    "battle_bridge_map.inc": "1d22d343719ae09642807010336172fdda7d8c2d37699b0e754f1ae2e1c74e72",
    "stubs.c": "d44d6e3480a5b07434678988bdb03544bc5802f35e77df987837df8a5b23e891",
}


def checked(*command, **kwargs):
    return subprocess.check_output(command, text=True, **kwargs)


def matching_symbols(path):
    symbols = {}
    for line in path.read_text().splitlines():
        obj, name, binding, *_ = line.split()
        symbols[(Path(obj.rstrip(":")).name, name)] = binding
    return symbols


def source_map(verified):
    result = {}
    for source in (ROOT / "src").rglob("*.c"):
        result[str(source.relative_to(ROOT)).replace("/", "_") + ".o"] = source
    for source in (ROOT / "src/battle").glob("*.c"):
        result["battle_host_" + source.name + ".o"] = source
    for source in (ROOT / "pc_port/src").rglob("*.c"):
        result[source.name + ".o"] = source
    result["port_main.o"] = ROOT / "pc_port/src/port_main.c"
    result["stubs.o"] = verified / "stubs.c"
    return result


def battle_renames(verified, objects):
    # The verified Linux pass namespaces battle symbols in these exact same TUs.
    # Apply the same names before code generation so Clang's COFF .refptr COMDATs
    # and declarations both carry the overlay-specific name.
    original_battle = set((verified / "battle_defined.txt").read_text().splitlines())
    original_other = set((verified / "port_defined.txt").read_text().splitlines())
    names = original_battle & original_other
    battle = [verified / "obj" / name for name in objects if name.startswith("battle_host_")]
    unresolved = checked("nm", "-A", "-P", "-u", *battle)
    for line in unresolved.splitlines():
        name = line.split(": ", 1)[1].split()[0]
        if re.fullmatch(r"battle_func_[0-9A-F]{8}", name):
            names.add(name.removeprefix("battle_"))
    if not names or any(not re.fullmatch(r"[A-Za-z_]\w*", name) for name in names):
        raise ValueError("invalid battle namespace manifest")
    return sorted(names)


def prepare(verified, out, objects):
    for name, expected in VERIFIED.items():
        actual = hashlib.sha256((verified / name).read_bytes()).hexdigest()
        if actual != expected:
            raise ValueError(f"r7.21 {name} mismatches verified input: {actual}")
    prior, after = (matching_symbols(verified / name) for name in ("matching-before.nm", "matching-after.nm"))
    if prior.keys() != after.keys():
        raise ValueError("symbol ownership manifests disagree")
    weak = {}
    for (obj, name), binding in prior.items():
        if binding == after[obj, name]:
            continue
        if binding != "T" or after[obj, name] != "W" or not re.fullmatch(r"\w+", name):
            raise ValueError(f"unexpected override binding for {obj} {name}")
        weak.setdefault(obj, []).append(name)
    for obj, names in weak.items():
        (out / (obj + ".h")).write_text("".join(f"#pragma weak {name}\n" for name in sorted(names)))
    names = battle_renames(verified, objects)
    (out / "battle_namespace.h").write_text("".join(f"#define {n} battle_{n}\n" for n in names))
    return weak


def generate_binding_table(out, objdir, objects):
    inspector = out / "coff_data_records"
    flags = shlex.split(checked("llvm-config-21", "--cxxflags", "--ldflags", "--libs", "object",
                                 "debuginfodwarf", "support", "--system-libs"))
    subprocess.run(["clang++-21", str(ROOT / "pc_port/tools/coff_data_records.cpp"), *flags, "-O2",
                    "-o", str(inspector)], check=True)
    cmd = [sys.executable, str(ROOT / "pc_port/tools/generate_win32_binding_sizes.py"),
           "--inspector", str(inspector), "--pe-evidence", str(out / "battle_win32_binding_evidence.json"), "--output", str(out / "battle_win32_binding_sizes.inc")]
    for source_name in ("archive_port.c", "data_archive_cd.c", "data_controller.c", "data_field.c", "data_slus_sdata.c"):
        cmd.extend(["--alias-source", str(ROOT / "pc_port/src" / source_name)])
    subprocess.run(cmd + [str(objdir / name) for name in objects], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verified", type=Path, required=True, help="r7.21 pc_port/build_native")
    parser.add_argument("--prefix", type=Path, required=True, help="Win64 SDL2/OpenAL SDK prefix")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--clang", default="clang-21")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--pattern", default=".*", help="only compile selected object names (diagnostic usage)")
    args = parser.parse_args()
    verified, prefix, out = (p.resolve() for p in (args.verified, args.prefix, args.out))
    out.mkdir(parents=True, exist_ok=True)
    objdir = out / "obj"
    objdir.mkdir(exist_ok=True)
    objects = [Path(line).name for line in (verified / "xeno-port.link-args").read_text().splitlines() if line.endswith(".o")]
    objects.append("battle_win32_host.c.o")
    if len(objects) != len(set(objects)):
        raise ValueError("duplicate production object name")
    sources = source_map(verified)
    if set(objects) - sources.keys():
        raise ValueError(f"unmapped production objects: {set(objects) - sources.keys()}")
    weak = prepare(verified, out, objects)
    zig_lib = Path(json.loads(checked("zig", "env"))["lib_dir"])
    sdk = ROOT / "pc_port/extern/PsyCross/include"
    target = [args.clang, "--target=x86_64-windows-gnu", "-isystem", str(zig_lib / "libc/include/any-windows-any")]
    common = target + [
        "-std=gnu17", "-DXENO_PC_PORT", "-DXENO_FIELD_OBJECT_OVERLAY", "-DSKIP_ASM", "-D_LANGUAGE_C",
        "-DUSE_EXTENDED_PRIM_POINTERS=0", "-include", "assert.h", "-include", str(TOOL_DIR / "host_psyq_types.h"),
        "-O0", "-g", "-gdwarf-4", "-fno-builtin", "-w", "-Wno-implicit-function-declaration",
        "-Wno-implicit-int", "-Wno-int-conversion", "-Wno-incompatible-pointer-types",
        "-Wno-incompatible-function-pointer-types", "-ffile-prefix-map=" + str(ROOT) + "=.",
        "-ffile-prefix-map=" + str(verified) + "=./pc_port/build_native",
        "-ffile-prefix-map=" + str(out) + "=./pc_port/build_win64",
        "-ffile-prefix-map=" + str(prefix) + "=./win64-sdk", "-ffile-prefix-map=" + str(zig_lib) + "=./mingw-sdk",
    ]
    battle_abi = set(checked(sys.executable, "tools/scripts/battle_overlay_host_tus.py", "--abi-tus", cwd=ROOT).splitlines())
    strict_flags = shlex.split(checked(sys.executable, "tools/scripts/battle_overlay_host_tus.py", "--abi-strict-flags", cwd=ROOT))
    selection = [o for o in objects if re.search(args.pattern, o)]

    def compile_one(name):
        source = sources[name]
        switches = list(common)
        # Retail EnterCriticalSection(void) masks VBlank. The Win32 function
        # takes a CRITICAL_SECTION pointer. SDK OS imports are compiled separately.
        if source.is_relative_to(ROOT / "src") or name in ("psyq_compat.c.o", "world_map_helper_762fc.c.o"):
            switches.append("-DEnterCriticalSection=XenoPsyQEnterCriticalSection")
        if name in weak:
            switches += ["-include", str(out / (name + ".h"))]
        if name.startswith("battle_host_"):
            switches += ["-DXENO_BATTLE_OVERLAY_HOST_BODIES", "-include", str(out / "battle_namespace.h")]
            if str(source.relative_to(ROOT)) in battle_abi:
                switches = [flag for flag in switches if flag != "-w"]
                switches += strict_flags
        if name == "src_slus_006.64_system_archive.c.o":
            switches.append("-DXENO_ARCHIVE_COMMAND_HANDLER_ONLY")
        includes = [ROOT / "pc_port/src", ROOT / "pc_port/include_shim", ROOT / "pc_port/include",
                    ROOT / "include", out, verified, sdk, sdk / "psx", prefix / "include", prefix / "include/SDL2"]
        if name == "port_main.o":
            includes = [p for p in includes if p not in {ROOT / "pc_port/include_shim", ROOT / "pc_port/include"}]
        cmd = switches + ["-I" + str(path) for path in includes] + ["-c", str(source), "-o", str(objdir / name)]
        result = subprocess.run(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (objdir / (name + ".log")).write_bytes(result.stdout)
        if result.returncode:
            (objdir / name).unlink(missing_ok=True)
        return name, result.returncode

    (out / "object-order.txt").write_text("\n".join(objects) + "\n")
    bridge = "battle_win32_host.c.o"
    initial = [name for name in selection if name != bridge]
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        failed = [name for name, code in pool.map(compile_one, initial) if code]
    if bridge in selection and not failed:
        generate_binding_table(out, objdir, [name for name in objects if name != bridge])
        name, code = compile_one(bridge)
        if code:
            failed.append(name)
    print(f"{len(selection) - len(failed)}/{len(selection)} Windows objects compiled")
    for name in failed:
        print(objdir / (name + ".log"))
    return bool(failed)


if __name__ == "__main__":
    sys.exit(main())
