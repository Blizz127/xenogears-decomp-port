#!/usr/bin/env python3
"""Run selected battle CPU characterizations against the player's extracted disc."""

import argparse
import hashlib
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


TESTS = Path(__file__).resolve().parent
PROJECT = TESTS.parents[1]
DISC = PROJECT / "disc"
MEDIA_DIGESTS = {
    "battle.bin": "1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291",
    "SLUS_006.64": "dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119",
}
RESIDENT_CASES = {"rot_matrix_z", "scale_matrix", "sound_enabled"}
FIXTURES = {
    "72a9c_label", "73380_digits", "73a58_popup", "73b64_submit", "73e88_panel",
    "742a0_phase", "745ec_inactive", "74d4c_inactive", "74eec_window", "75168_refresh_fault",
    "75938_gauge", "75938_refresh", "99890_inactive", "command_menu_default",
    "command_menu_early_exit", "command_menu_fault", "command_menu_four_state",
    "command_menu_two_frame", "frame_geometry", "frame_sprite", "menu_camera", "menu_sound",
    "menu_sound_fault", "rot_matrix_z", "scale_matrix", "sound_enabled",
}


def media(fixture):
    return "SLUS_006.64" if fixture in RESIDENT_CASES else "battle.bin"


def check_media(name):
    digest = hashlib.sha256()
    with (DISC / name).open("rb") as stream:
        for chunk in iter(lambda: stream.read(128 * 1024), b""):
            digest.update(chunk)
    if digest.hexdigest() != MEDIA_DIGESTS[name]:
        raise SystemExit(f"Expected the supported disc revision for {name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixtures", nargs="+", choices=sorted(FIXTURES))
    args = parser.parse_args()
    for name in sorted({media(test) for test in args.fixtures}):
        check_media(name)

    env = os.environ.copy()
    env.pop("XENO_DISABLE_CPU_INTERPRETER", None)
    compiler = shlex.split(os.environ.get("CC", "cc"))
    # Two target-process cases explicitly check whether a 32-bit pointer can
    # name a low host address. A non-PIE fixture reproduces their test setup.
    compile_cmd = compiler + ["-std=gnu17", "-O0", "-fno-pie", "-DXENO_PC_PORT", "-Ipc_port/src"]
    with tempfile.TemporaryDirectory(prefix="xeno-battle-guest-") as work:
        adapter = Path(work) / "battle_mips_adapter.o"
        subprocess.run(compile_cmd + ["-c", "pc_port/src/battle_mips_adapter.c", "-o", adapter],
                       cwd=PROJECT, check=True, timeout=60)
        for fixture in args.fixtures:
            executable = Path(work) / fixture
            source = TESTS / f"battle_{fixture}_test.c"
            subprocess.run(compile_cmd + ["-no-pie", source, adapter, "-o", executable],
                           cwd=PROJECT, check=True, timeout=60)
            subprocess.run([executable, DISC / media(fixture)], cwd=PROJECT,
                           env=env, check=True, timeout=60)
            print(f"{fixture}: guest checks passed", flush=True)


if __name__ == "__main__":
    main()
