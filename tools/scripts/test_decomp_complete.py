#!/usr/bin/env python3
"""Gate: every overlay in the decomp universe is MATCHED or COEXISTENCE.

Drives tools/scripts/decomp_status.py (the shipped dashboard) plus a
src-vs-asm scan of battle so Unported cannot be zeroed by deleting
INCLUDE_ASM without leaving a C body.
"""
from __future__ import print_function

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)

import decomp_status  # shipped oracle


INCLUDE_RE = re.compile(r'INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*([A-Za-z0-9_]+)\s*\)')
# Macro signature: #define XBT_FOO u32 func_80083FF4(
MACRO_SIG_RE = re.compile(
    r'#define[ \t]+\w+[ \t]+(?:[\w\*]+[ \t]+)+([A-Za-z_][\w]*)\s*\('
)
# Ordinary C definition at column 0 (not a call, not a prototype-only extern
# buried in a block). Matches "void foo(" / "u32 foo(" / "static s32 foo(".
C_DEF_RE = re.compile(
    r'^[ \t]*(?:(?:static|inline)\s+)*(?:unsigned\s+)?(?:void|u8|u16|u32|s8|s16|s32|int|long|char|short)\s*\*?\s*([A-Za-z_][\w]*)\s*\(',
    re.M,
)


def battle_universe(root):
    names = set()
    asm = os.path.join(root, "asm", "battle")
    for split in ("matchings", "nonmatchings"):
        base = os.path.join(asm, split)
        if not os.path.isdir(base):
            continue
        for dirpath, _, files in os.walk(base):
            for name in files:
                if name.endswith(".s"):
                    names.add(name[:-2])
    return names


def battle_defined_names(root):
    include_names = set()
    c_names = set()
    src = os.path.join(root, "src", "battle")
    for dirpath, _, files in os.walk(src):
        for name in files:
            if not name.endswith((".c", ".inc", ".h")):
                continue
            path = os.path.join(dirpath, name)
            text = open(path, errors="replace").read()
            for match in INCLUDE_RE.finditer(text):
                include_names.add(match.group(2))
            for match in MACRO_SIG_RE.finditer(text):
                c_names.add(match.group(1))
            for match in C_DEF_RE.finditer(text):
                c_names.add(match.group(1))
    return include_names, c_names


def main():
    rows, per_func, _have_stubs, _n_src, _stubs, _univ, _src_status = (
        decomp_status.classify(ROOT)
    )
    tot = {"matched": 0, "coexistence": 0, "unported": 0, "total": 0}
    ov = {}
    for (overlay, _module), counts in rows.items():
        bucket = ov.setdefault(
            overlay, {"matched": 0, "coexistence": 0, "unported": 0, "total": 0}
        )
        for key in tot:
            tot[key] += counts[key]
            bucket[key] += counts[key]

    errors = []
    if tot["total"] != 3295:
        errors.append("universe total %s != 3295" % tot["total"])
    if tot["unported"] != 0:
        errors.append("TOTAL unported %s != 0" % tot["unported"])
    if tot["matched"] + tot["coexistence"] != tot["total"]:
        errors.append(
            "matched+coex %s != total %s"
            % (tot["matched"] + tot["coexistence"], tot["total"])
        )
    battle = ov.get("battle", {})
    if battle.get("unported", -1) != 0:
        errors.append("battle unported %s != 0" % battle.get("unported"))
    if battle.get("matched", 0) + battle.get("coexistence", 0) != battle.get("total", -1):
        errors.append("battle not 100%% done: %s" % battle)
    for overlay in (
        "field",
        "member_change_menu",
        "menu",
        "shop_menu",
        "slus_006.64",
        "battle",
    ):
        if ov.get(overlay, {}).get("unported", -1) != 0:
            errors.append("%s unported %s" % (overlay, ov[overlay]["unported"]))

    universe = battle_universe(ROOT)
    include_names, c_names = battle_defined_names(ROOT)
    missing = sorted(universe - include_names - c_names)
    if missing:
        errors.append("battle functions with neither INCLUDE_ASM nor C: %s" % missing[:20])

    print("universe", tot["total"])
    print("unported", tot["unported"])
    print("matched", tot["matched"])
    print("coexistence", tot["coexistence"])
    print("battle", battle)
    print("battle_universe", len(universe))
    print("battle_missing_defs", len(missing))
    if errors:
        print("FAIL")
        for err in errors:
            print(" ", err)
        return 1
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
