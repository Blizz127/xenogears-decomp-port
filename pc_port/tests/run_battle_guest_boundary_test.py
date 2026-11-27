#!/usr/bin/env python3
"""Test the battle bridge on Linux x86_64 using two prepared, user-owned US Disc 1 files."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2],
                        help="source checkout used to produce the bridge map")
    parser.add_argument("--renderer", type=Path, required=True,
                        help="prepared PsyCross source from the same build")
    parser.add_argument("--bridge-map", type=Path, required=True,
                        help="battle_bridge_map.inc from the regular port build")
    parser.add_argument("--media-dir", type=Path, required=True,
                        help="directory with US Disc 1 battle.bin and battle_module_0c4.bin")
    parser.add_argument("--slus", type=Path,
                        help="also exercise retained resident services using the US Disc 1 SLUS_006.64")
    parser.add_argument("--work-dir", type=Path, required=True,
                        help="existing on-disk directory for temporary build files")
    parser.add_argument("--cc", default="gcc")
    parser.add_argument("--objcopy", default="objcopy", help="GNU-compatible objcopy for the optional resident cases")
    args = parser.parse_args()
    root = args.root.resolve()
    port = root / "pc_port"
    renderer = args.renderer.resolve()
    bridge = args.bridge_map.resolve()
    if bridge.name != "battle_bridge_map.inc" or not bridge.is_file():
        parser.error("--bridge-map must name the generated battle_bridge_map.inc")
    if not (renderer / "src/gpu/PsyX_GPU.h").is_file():
        parser.error("--renderer must contain the prepared renderer headers")
    inputs = []
    for filename, expected in (
        ("battle.bin", "1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291"),
        ("battle_module_0c4.bin", "4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885"),
    ):
        path = (args.media_dir / filename).resolve()
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            parser.error(f"missing or unrecognized US Disc 1 input: {path}")
        inputs.append(path)

    if args.slus:
        executable = args.slus.resolve()
        expected = "dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
        if not executable.is_file() or hashlib.sha256(executable.read_bytes()).hexdigest() != expected:
            parser.error("--slus must name the US Disc 1 SLUS_006.64 executable")
        inputs.append(executable)

    with tempfile.TemporaryDirectory(prefix="battle-guest-", dir=args.work_dir) as work:
        binary = Path(work) / "battle-guest-test"
        include_dirs = (port / "src", port / "include_shim", port / "include",
                        root / "include", bridge.parent, renderer / "include",
                        renderer / "include/psx", renderer / "src")
        flags = ["-std=gnu17", "-m64", "-O0", "-fno-builtin", "-fno-pie", "-no-pie",
                 "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                 "-DXENO_PC_PORT", "-DXENO_FIELD_OBJECT_OVERLAY", "-DSKIP_ASM",
                 "-D_LANGUAGE_C", "-DUSE_EXTENDED_PRIM_POINTERS=0"]
        fixture = Path(__file__).with_name("battle_guest_boundary_test.c")
        flags.extend("-I" + str(p) for p in include_dirs)
        objects = []
        if args.slus:
            flags.append("-DTEST_RETAINED_SERVICES")
            # The differential calls only these production providers. Localizing
            # the other symbols allows the regular section-GC link to discard them.
            services = (
                ("mask", root / "src/battle/main41.c",
                 ["-include", "common.h", "-include", "psx_memory.h", "-include", "battle_overlay_guest_ram.h"],
                 ["func_80089C08"]),
                ("range", root / "src/slus_006.64/system/temp3.c", ["-include", "stdlib.h"], ["func_8001BD40"]),
                ("compat", port / "src/psyq_compat.c", [], ["rand", "g_RandomSeed"]),
            )
            for name, source, extra, symbols in services:
                obj = Path(work) / (name + ".o")
                subprocess.run([args.cc, *flags, "-Wno-int-conversion", "-Wno-incompatible-pointer-types", "-Wno-implicit-function-declaration",
                                *extra, "-c", source, "-o", obj], cwd=root, check=True)
                subprocess.run([args.objcopy, *["--keep-global-symbol=" + sym for sym in symbols], obj], check=True)
                objects.append(obj)
        subprocess.run([args.cc, *flags,
                        fixture, port / "src/battle_mips_adapter.c", *objects, "-ldl", "-lm", "-pthread",
                        "-o", binary], cwd=root, check=True)
        # Feature overrides in a developer shell must not select a different mode.
        env = {k: v for k, v in os.environ.items() if not k.startswith("XENO_")}
        env["XENO_CPU_EXECUTION_STATS"] = "1"
        for disabled in ("0", "1"):
            subprocess.run([binary, *inputs], cwd=work, env=dict(env, XENO_DISABLE_CPU_INTERPRETER=disabled),
                           check=True, timeout=20)


if __name__ == "__main__":
    main()
