#!/usr/bin/env python3
"""One-shot codemod: propose battle guest-ABI edits for named functions.

M-a, battle-abi-design.md section 2 (approach (a), with (b) only as a codemod
whose output is reviewed and committed).  For each named function in
src/battle it rewrites the mechanical P2 pattern and reports every remaining
site a reviewer must still look at; nothing is adopted automatically.

Rewrites (only inside the named function's definition):
  - parameter  `T** name`         -> `GSLOT(T) name`
  - expression `*name` (that slot) -> `GLOAD(T, name)`
Reports (not rewritten): any other `**`, `*(void **)`, `*(T **)`, `(u32)D_`,
`(s32)D_`, int/pointer casts on D_ symbols, `asm("$..")` register pins.

The file must include "battle_guest_abi.h" (added when missing).  After the
edit: add a typed thunk (pc_port/src/battle_guest_abi_thunks.inc), the leaf
(battle_overlay_host_leaves.inc), a differential case and a mutant, and run
`make rom-check` (the macros must leave the matching build byte-identical).

Usage: battle_guest_abi_codemod.py [--dry-run] func_XXXXXXXX ...
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BATTLE_SRC = ROOT / "src" / "battle"
HEADER = '#include "battle_guest_abi.h"'
REVIEW_RES = [
    (re.compile(r"\*\s*\(\s*\w+\s*\*\*\s*\)"), "pointer-to-pointer cast (P3)"),
    (re.compile(r"\(\s*[us]32\s*\)\s*D_"), "int cast of a D_ symbol (P5/P4)"),
    (re.compile(r"\(\s*\w+\s*\*\s*\)\s*\(\s*[us]32\s*\)"), "int->pointer round trip (P4)"),
    (re.compile(r"\w\s*\*\*\s*\w"), "remaining T** (P2)"),
    (re.compile(r'asm\s*\(\s*"\$'), "MIPS register pin (P9)"),
]


def find_definition(text: str, name: str) -> tuple[int, int, int] | None:
    """(start of the definition line, '(' of the parameter list, closing '}')."""
    m = re.search(rf"^[^\n;]*\b{name}\s*\(", text, re.M)
    while m is not None:
        paren = m.end() - 1
        depth = 0
        close = paren
        for i in range(paren, len(text)):
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    close = i
                    break
        rest = text[close + 1:].lstrip()
        if rest.startswith("{"):
            body = text.index("{", close)
            depth = 0
            for i in range(body, len(text)):
                if text[i] == "{":
                    depth += 1
                elif text[i] == "}":
                    depth -= 1
                    if depth == 0:
                        return m.start(), paren, i
        m = re.compile(rf"^[^\n;]*\b{name}\s*\(", re.M).search(text, m.end())
    return None


def convert(text: str, name: str) -> tuple[str, list[str]]:
    found = find_definition(text, name)
    if found is None:
        raise SystemExit(f"{name}: no definition in this file")
    start, paren, end = found
    seg = text[start:end + 1]
    notes: list[str] = []
    close = seg.index(")", paren - start)
    params = seg[paren - start + 1:close]
    new_params = params
    slots: dict[str, str] = {}
    for pm in re.finditer(r"\b(\w+)\s*\*\*\s*(\w+)", params):
        t, var = pm.group(1), pm.group(2)
        slots[var] = t
        new_params = new_params.replace(pm.group(0), f"GSLOT({t}) {var}", 1)
    body = seg[close:]
    for var, t in slots.items():
        body, n = re.subn(rf"(?<![\w)\]])\*\s*{var}\b(?!\s*\[)", f"GLOAD({t}, {var})", body)
        notes.append(f"{name}: slot {var} ({t}**) -> GSLOT/GLOAD x{n}")
    seg = seg[:paren - start + 1] + new_params + body
    for rx, what in REVIEW_RES:
        for hit in rx.finditer(seg):
            line = text[:start].count("\n") + seg[:hit.start()].count("\n") + 1
            notes.append(f"{name}: REVIEW line {line}: {what}: {hit.group(0)!r}")
    return text[:start] + seg + text[end + 1:], notes


def main() -> int:
    dry = "--dry-run" in sys.argv
    names = [a for a in sys.argv[1:] if a.startswith("func_")]
    if not names:
        print(__doc__)
        return 2
    by_file: dict[Path, list[str]] = {}
    for name in names:
        owners = [p for p in sorted(BATTLE_SRC.glob("*.c"))
                  if find_definition(p.read_text(errors="ignore"), name)]
        if len(owners) != 1:
            print(f"{name}: {len(owners)} defining files; resolve by hand")
            return 1
        by_file.setdefault(owners[0], []).append(name)
    for path, fns in by_file.items():
        text = path.read_text()
        for name in fns:
            text, notes = convert(text, name)
            for note in notes:
                print(f"{path.relative_to(ROOT)}: {note}")
        if HEADER not in text:
            text = text.replace('#include "common.h"\n', f'#include "common.h"\n{HEADER}\n', 1)
            print(f"{path.relative_to(ROOT)}: added {HEADER}")
        if not dry:
            path.write_text(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
