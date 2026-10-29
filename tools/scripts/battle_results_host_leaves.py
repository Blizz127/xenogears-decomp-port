#!/usr/bin/env python3
"""Read and check the adopted battle RESULTS module allowlist.

pc_port/src/battle_results_host_leaves.inc lists the results-module routines
(retail overlay linked at 0x801DE000) the port may run as native C.  Each
entry is ``{0x801exxxxu, "func_801EXXXX"},`` keyed by the routine's real jal
target; pc_port/src/battle_results_host.c must define the body as
``battle_results_func_801EXXXX``.  build_port.sh and
pc_port/tests/run_battle_results_host_differential_test.sh both take the list
from here, so the names and the checks cannot drift.

Usage:
    tools/scripts/battle_results_host_leaves.py --verify    # check, print names
    tools/scripts/battle_results_host_leaves.py --leaves    # func_ names
    tools/scripts/battle_results_host_leaves.py --symbols   # host body symbols
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LEAVES = ROOT / "pc_port" / "src" / "battle_results_host_leaves.inc"

RESULTS_BASE = 0x801DE000
# Immutable code/rodata; 0x801E44C0.. is mutable module data (see
# pc_port/src/battle_results_gate.h).
RESULTS_IMMUTABLE_END = 0x801E44C0

ENTRY_RE = re.compile(r'\{\s*(0x[0-9A-Fa-f]+)u?\s*,\s*"(func_[0-9A-Fa-f]+)"\s*\}\s*,')
QUOTED_RE = re.compile(r'"(func_\w+)"')


def entries(path: Path = LEAVES) -> list[tuple[int, str]]:
    text = path.read_text()
    found = [(int(a, 16), n) for a, n in ENTRY_RE.findall(text)]
    quoted = QUOTED_RE.findall(text)
    if len(quoted) != len(found):
        raise SystemExit(
            f"{path}: {len(quoted)} quoted func_ names but {len(found)} "
            "well-formed {address, name} entries")
    return found


def verify(found: list[tuple[int, str]]) -> list[str]:
    problems = []
    seen = set()
    for address, name in found:
        if name != f"func_{address:08X}":
            problems.append(f"{name}: name does not spell its address 0x{address:08X}")
        if address & 3:
            problems.append(f"{name}: unaligned entry address")
        if not RESULTS_BASE <= address < RESULTS_IMMUTABLE_END:
            problems.append(f"{name}: outside results code 0x{RESULTS_BASE:08X}.."
                            f"0x{RESULTS_IMMUTABLE_END:08X}")
        if address in seen:
            problems.append(f"{name}: duplicate entry")
        seen.add(address)
    return problems


def main() -> int:
    mode = sys.argv[1] if len(sys.argv) > 1 else "--verify"
    found = entries()
    if mode == "--verify":
        problems = verify(found)
        for problem in problems:
            print(f"battle_results_host_leaves: {problem}", file=sys.stderr)
        if problems:
            return 1
        for _, name in found:
            print(name)
        return 0
    if mode == "--leaves":
        for _, name in found:
            print(name)
        return 0
    if mode == "--symbols":
        for _, name in found:
            print(f"battle_results_{name}")
        return 0
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
