#!/usr/bin/env python3
"""
Decomp-completeness dashboard (read-only burn-down).

Rolls up every retail function into four categories, per translation unit and
project-wide, from the repo's existing source-side signals. Changes nothing.

    python3 tools/scripts/decomp_status.py            # markdown table to stdout
    python3 tools/scripts/decomp_status.py --json out.json   # + machine-readable

Categories (matching-build view -- "is the retail function decompiled?"):
  MATCHED {}   : C body in src, no INCLUDE_ASM for it. Byte-exact is INFERRED
                 from the project convention (only {} lands as a plain C body;
                 near-matches use coexistence). No objdiff oracle is consulted
                 -- build/progress.json is absent (the `make report` mv-bug),
                 so this is the one inferred signal. Flagged in the report.
  COEXISTENCE  : matching build stays byte-exact via INCLUDE_ASM while the port
                 has a real body. Two shapes, both detected:
                   - inline: INCLUDE_ASM inside a matching-only preprocessor
                     branch (`#else` of `#ifdef XENO_PC_PORT`, or `#ifndef
                     XENO_PC_PORT`). e.g. the 2 sound init functions.
                   - file-level: the whole TU is port-replaced by a *_port.c
                     (work_list.c -> work_list_port.c, archive.c ->
                     archive_port.c); its INCLUDE_ASM functions are coexistence.
  UNPORTED     : unconditional INCLUDE_ASM. Matching = asm; the port's empty
                 INCLUDE_ASM macro leaves it undefined -> auto-stubbed.
  (MATCHED + COEXISTENCE = "done for the matching build"; both are byte-exact
   there. UNPORTED is the remaining work.)

Port view (separate, different denominator -- "what the port RUNS"):
  Excludes psyq/** (PsyCross provides those). A function is REAL-IN-PORT if
  MATCHED, COEXISTENCE, or in a port-replaced file; STUBBED if it appears in the
  generated pc_port/build_native/stubs.c oracle manifest.

Universe: every function has exactly one .s under asm/<overlay>/{matchings,
nonmatchings}/**. That union is the complete function list (the matchings vs
nonmatchings SPLIT is a lagging indicator -- a just-matched function stays in
nonmatchings until a resync -- so the split is NOT used for status; only the
union is used, as the denominator).
"""

import argparse
import json
import os
import re
import sys
from collections import defaultdict

INCLUDE_ASM_RE = re.compile(r'INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*([A-Za-z0-9_]+)\s*\)')
STUB_FUNC_RE = re.compile(r'^\s*(?:long|void|unsigned\s+\w+|int)\s+([A-Za-z_]\w*)\s*\(void\)\s*\{\s*xeno_port_stub', re.M)
STUB_DATA_RE = re.compile(r'^\s*unsigned\s+char\s+([A-Za-z_]\w*)\s*\[', re.M)

# Port TU handling (from pc_port/build_port.sh).
PORT_EXCLUDED_PATTERNS = ("/psyq/",)                       # replaced by PsyCross
PORT_REPLACED_FILES = (                                     # matching src, port *_port.c
    "src/slus_006.64/system/work_list.c",
    "src/slus_006.64/system/archive.c",
)


def repo_root():
    p = os.path.abspath(os.path.dirname(__file__))
    while p != "/":
        if os.path.isfile(os.path.join(p, "gears.toml")):
            return p
        p = os.path.dirname(p)
    return os.getcwd()


def build_universe(root):
    """(overlay, module) -> set(funcname), from the asm .s union."""
    universe = defaultdict(set)
    func_module = {}  # (overlay, funcname) -> module (for INCLUDE_ASM cross-ref)
    asm_dir = os.path.join(root, "asm")
    for overlay in sorted(os.listdir(asm_dir)):
        ov_path = os.path.join(asm_dir, overlay)
        if not os.path.isdir(ov_path):
            continue
        for split in ("matchings", "nonmatchings"):
            base = os.path.join(ov_path, split)
            if not os.path.isdir(base):
                continue
            for dirpath, _, files in os.walk(base):
                module = os.path.relpath(dirpath, base)
                for f in files:
                    if f.endswith(".s"):
                        fn = f[:-2]
                        universe[(overlay, module)].add(fn)
                        func_module[(overlay, fn)] = module
    return universe, func_module


def scan_preproc_coexistence(text):
    """Return the set of INCLUDE_ASM funcnames that sit in a matching-only
    (port-excluded) preprocessor branch => inline coexistence. Tracks nested
    #if/#ifdef/#ifndef with XENO_PC_PORT awareness and #else flips."""
    coex = set()
    # stack frames: 'active_excludes_port' True if current branch compiles only
    # when XENO_PC_PORT is NOT defined.
    stack = []  # list of dict(kind, port_only, matching_only)
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("#if"):
            m = re.match(r'#\s*(ifdef|ifndef|if)\s+(.*)', s)
            expr = m.group(2) if m else ""
            directive = m.group(1) if m else "if"
            is_xeno = "XENO_PC_PORT" in expr
            if directive == "ifdef":
                po, mo = (is_xeno, False)          # active when defined
            elif directive == "ifndef":
                po, mo = (False, is_xeno)           # active when NOT defined
            else:  # #if defined / #if !defined
                neg = "!" in expr or "!defined" in expr.replace(" ", "")
                if is_xeno:
                    po, mo = (not neg, neg)
                else:
                    po, mo = (False, False)
            stack.append({"port_only": po, "matching_only": mo})
        elif s.startswith("#else"):
            if stack:
                top = stack[-1]
                top["port_only"], top["matching_only"] = top["matching_only"], top["port_only"]
        elif s.startswith("#elif"):
            if stack:
                stack[-1] = {"port_only": False, "matching_only": False}
        elif s.startswith("#endif"):
            if stack:
                stack.pop()
        else:
            m = INCLUDE_ASM_RE.search(line)
            if m and any(fr["matching_only"] for fr in stack):
                coex.add(m.group(2))
    return coex


def overlay_of_asm_path(asm_path):
    # INCLUDE_ASM paths are often relative ("../asm/menu/..."), so search for
    # the asm/<overlay>/ component instead of assuming a prefix.
    m = re.search(r"asm/([^/]+)/", asm_path)
    return m.group(1) if m else "?"


def scan_sources(root):
    """(overlay, funcname) -> dict(status='COEXISTENCE'|'UNPORTED', file).

    Keyed by overlay because overlays share load addresses: e.g. field's
    func_8007DCF8 and battle's func_8007DCF8 are different functions that
    previously collapsed onto one global name entry.
    """
    status = {}
    for dirpath, _, files in os.walk(os.path.join(root, "src")):
        for f in files:
            if not f.endswith(".c"):
                continue
            path = os.path.join(dirpath, f)
            rel = os.path.relpath(path, root)
            with open(path, errors="replace") as fh:
                text = fh.read()
            if "INCLUDE_ASM" not in text:
                continue
            inline_coex = scan_preproc_coexistence(text)
            file_level_coex = rel in PORT_REPLACED_FILES
            for m in INCLUDE_ASM_RE.finditer(text):
                asm_path, fn = m.group(1), m.group(2)
                overlay = overlay_of_asm_path(asm_path)
                if fn in inline_coex or file_level_coex:
                    st = "COEXISTENCE"
                else:
                    st = "UNPORTED"
                # first definition wins; prefer COEXISTENCE if any file says so
                key = (overlay, fn)
                if key not in status or (st == "COEXISTENCE" and status[key]["status"] != "COEXISTENCE"):
                    status[key] = {"status": st, "file": rel, "overlay": overlay,
                                   "file_level": file_level_coex and st == "COEXISTENCE"}
    return status


def load_stubs(root):
    path = os.path.join(root, "pc_port/build_native/stubs.c")
    if not os.path.isfile(path):
        return set(), set(), False
    text = open(path, errors="replace").read()
    # drop the xeno_port_stub definition itself (not a stub)
    funcs = set(STUB_FUNC_RE.findall(text)) - {"xeno_port_stub"}
    return funcs, set(STUB_DATA_RE.findall(text)), True


def matchings_dir_count(root):
    """Cross-check signal: functions the LAST resync placed in matchings/ (a
    lagging match indicator). matched_inferred should ~= this + recently-matched
    still sitting in nonmatchings/."""
    total = 0
    for dp, _, fs in os.walk(os.path.join(root, "asm")):
        if os.sep + "matchings" + os.sep in dp + os.sep or dp.endswith(os.sep + "matchings"):
            if "nonmatchings" not in dp:
                total += sum(1 for f in fs if f.endswith(".s"))
    return total


def classify(root):
    universe, _ = build_universe(root)
    src_status = scan_sources(root)
    stub_funcs, _, have_stubs = load_stubs(root)

    rows = {}  # (overlay, module) -> counts
    per_func = []
    for (overlay, module), funcs in universe.items():
        c = {"matched": 0, "coexistence": 0, "unported": 0,
             "coex_inline": 0, "coex_file": 0, "stubbed": 0, "total": 0}
        is_psyq = "psyq" in module or module.startswith("psyq")
        is_replaced = any(overlay == "slus_006.64" and module == m for m in
                          ("system/work_list", "system/archive"))
        for fn in funcs:
            c["total"] += 1
            info = src_status.get((overlay, fn))
            if info and info["status"] == "COEXISTENCE":
                st = "coexistence"
                if info["file_level"]:
                    c["coex_file"] += 1
                else:
                    c["coex_inline"] += 1
            elif info and info["status"] == "UNPORTED":
                st = "unported"
            else:
                st = "matched"
            c[st] += 1
            stubbed = fn in stub_funcs
            if stubbed:
                c["stubbed"] += 1
            per_func.append({"overlay": overlay, "module": module, "func": fn,
                             "status": st, "stubbed": stubbed, "psyq": is_psyq})
        # file-level replacement: mark whole-TU unported funcs as port-real
        c["port_excluded"] = is_psyq
        c["port_replaced"] = is_replaced
        rows[(overlay, module)] = c
    universe_funcs = set(fn for funcs in universe.values() for fn in funcs)
    return rows, per_func, have_stubs, len(src_status), stub_funcs, universe_funcs, src_status


def fmt_pct(n, d):
    return f"{100.0 * n / d:5.1f}%" if d else "   -  "


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", help="also write machine-readable JSON here")
    ap.add_argument("--min-total", type=int, default=1,
                    help="hide TU rows with fewer than N functions")
    args = ap.parse_args()

    root = repo_root()
    rows, per_func, have_stubs, n_src_status, stub_funcs, universe_funcs, src_status = classify(root)

    # ---- roll-ups ----
    tot = defaultdict(int)
    ov_tot = defaultdict(lambda: defaultdict(int))
    for (overlay, module), c in rows.items():
        for k in ("matched", "coexistence", "unported", "coex_inline",
                  "coex_file", "stubbed", "total"):
            tot[k] += c[k]
            ov_tot[overlay][k] += c[k]

    out = []
    out.append("# Decomp Completeness Dashboard\n")
    out.append(f"_Read-only snapshot. Universe = {tot['total']} functions "
               f"(asm .s union across overlays)._\n")
    out.append("Categories: **MATCHED {}** (C body, no INCLUDE_ASM; byte-exact "
               "*inferred*), **COEX** (matching byte-exact via INCLUDE_ASM, port "
               "has a real body), **UNPORTED** (unconditional INCLUDE_ASM). "
               "MATCHED+COEX = done-for-matching-build.\n")

    # ---- per-overlay summary ----
    out.append("\n## Per-overlay\n")
    out.append("| Overlay | Total | Matched {} | Coex | Unported | %{} | %done(m+c) |")
    out.append("|---|--:|--:|--:|--:|--:|--:|")
    for overlay in sorted(ov_tot):
        c = ov_tot[overlay]
        done = c["matched"] + c["coexistence"]
        out.append(f"| {overlay} | {c['total']} | {c['matched']} | "
                   f"{c['coexistence']} | {c['unported']} | "
                   f"{fmt_pct(c['matched'], c['total'])} | {fmt_pct(done, c['total'])} |")
    done = tot["matched"] + tot["coexistence"]
    out.append(f"| **TOTAL** | **{tot['total']}** | **{tot['matched']}** | "
               f"**{tot['coexistence']}** | **{tot['unported']}** | "
               f"**{fmt_pct(tot['matched'], tot['total'])}** | "
               f"**{fmt_pct(done, tot['total'])}** |")

    # ---- per-TU table (sorted by unported desc, then coex desc) ----
    out.append("\n## Per-TU (translation unit / module)\n")
    out.append("| Overlay / module | Total | M{} | Coex(inline/file) | Unported | Stub | %done |")
    out.append("|---|--:|--:|--:|--:|--:|--:|")
    def sort_key(item):
        (ov, mod), c = item
        return (-c["unported"], -c["coexistence"], ov, mod)
    for (overlay, module), c in sorted(rows.items(), key=sort_key):
        if c["total"] < args.min_total:
            continue
        if c["unported"] == 0 and c["coexistence"] == 0 and c["total"] > 0:
            continue  # fully-matched TUs: omit from the burn-down (see --min-total)
        done = c["matched"] + c["coexistence"]
        coexstr = f"{c['coexistence']}" + (f" ({c['coex_inline']}i/{c['coex_file']}f)"
                                           if c["coexistence"] else "")
        out.append(f"| {overlay}/{module} | {c['total']} | {c['matched']} | "
                   f"{coexstr} | {c['unported']} | {c['stubbed']} | "
                   f"{fmt_pct(done, c['total'])} |")

    # fully-matched TU count (omitted above)
    full = sum(1 for c in rows.values() if c["unported"] == 0 and c["coexistence"] == 0 and c["total"] > 0)
    out.append(f"\n_({full} TUs are fully MATCHED {{}} with no coex/unported -- omitted from the burn-down list above.)_")

    # ---- port view ----
    out.append("\n## Port view (what the PORT runs: real vs stubbed)\n")
    port_real = port_stub = port_excl = 0
    for pf in per_func:
        if pf["psyq"]:
            port_excl += 1
            continue
        # real in port = matched, coexistence, or (module port-replaced)
        c = rows[(pf["overlay"], pf["module"])]
        is_replaced = c["port_replaced"]
        if pf["status"] in ("matched", "coexistence") or is_replaced:
            port_real += 1
        elif pf["stubbed"]:
            port_stub += 1
        else:
            # unported, not stubbed, not replaced: not referenced by the port
            pass
    n_stubs_total = len(stub_funcs)
    stubs_in_univ = len(stub_funcs & universe_funcs)
    stubs_out_univ = n_stubs_total - stubs_in_univ
    port_touched = port_real + n_stubs_total   # functions the port links/references
    out.append(f"- Port-relevant functions (psyq/** excluded from the real count: {port_excl}).")
    out.append(f"- **Real in port** (matched + coexistence + port-replaced): {port_real}")
    out.append(f"- **Oracle-stubbed** in port: {n_stubs_total} function stubs"
               + ("" if have_stubs else "  [stubs.c ABSENT -- build the port first]"))
    if have_stubs:
        out.append(f"- Port \"runs\" {port_touched} functions -> "
                   f"**{fmt_pct(port_real, port_touched)} real**, "
                   f"{fmt_pct(n_stubs_total, port_touched)} stubbed.")

    # ---- reconciliation ----
    out.append("\n## Reconciliation vs known anchors\n")
    total_incl_asm = tot["coexistence"] + tot["unported"]
    mdir = matchings_dir_count(root)
    checks = []
    # src_status is keyed by (overlay, fn); only pairs inside the disassembled
    # universe can be classified, the rest (e.g. battling refs with no asm/
    # dir) are reported separately.
    universe_pairs = set((pf["overlay"], pf["func"]) for pf in per_func)
    in_universe = sum(1 for k in src_status if k in universe_pairs)
    out_universe = n_src_status - in_universe
    checks.append((total_incl_asm == in_universe,
                   f"INCLUDE_ASM'd funcs (coex {tot['coexistence']} + unported "
                   f"{tot['unported']}) = {total_incl_asm} == distinct in-universe "
                   f"(overlay, fn) INCLUDE_ASM pairs in src ({in_universe}; "
                   f"{out_universe} pairs reference overlays outside the asm/ tree, "
                   f"e.g. battling)"))
    # matched inference vs lagging matchings/ dir (recently-matched still sit in
    # nonmatchings, so matched_inferred >= mdir by that small delta).
    checks.append((tot["matched"] >= mdir,
                   f"matched (inferred) {tot['matched']} >= matchings/ dir count "
                   f"{mdir} (delta {tot['matched'] - mdir} = recently-matched still "
                   f"in nonmatchings/, e.g. the 2 sound init fns)"))
    # func_800799D4 (d88f13c): fully decompiled into src/field/main/misc4.c,
    # so the field pair must classify MATCHED (a plain C body, no INCLUDE_ASM).
    d4 = next((pf for pf in per_func if pf["func"] == "func_800799D4"
               and pf["overlay"] == "field"), None)
    checks.append((d4 is not None and d4["status"] == "matched",
                   f"func_800799D4 (d88f13c anchor) classified MATCHED for "
                   f"field: {d4['status'] if d4 else 'NOT FOUND'}"))
    if have_stubs:
        checks.append((True,
                       f"stubs: {n_stubs_total} total; {stubs_in_univ} in the "
                       f"4-overlay universe, {stubs_out_univ} outside it "
                       f"(segments not disassembled into asm/ -- e.g. 0x801Cxxxx "
                       f"overlays). Not a bug; a coverage limit."))
    for ok, msg in checks:
        out.append(f"- [{'OK' if ok else '!!'}] {msg}")

    # ---- limitations / inference flags ----
    out.append("\n## Limitations (what is inferred vs authoritative)\n")
    out.append("- **MATCHED {} is inferred, not oracle-verified.** No "
               "build/progress.json (the `make report` mv-bug), so `MATCHED = "
               "universe - INCLUDE_ASM`. This assumes every non-INCLUDE_ASM'd "
               "function is byte-exact (project convention: only {} lands as a "
               "plain C body; near-matches use coexistence). Cross-checked "
               "against the matchings/ dir above. To make it authoritative, run "
               "`make report` and diff build/progress.json.")
    out.append("- **Universe = the 4 disassembled overlays** (slus_006.64, "
               "field, member_change_menu, shop_menu). Functions the port stubs "
               "from other segments (battle/other overlays, ~0x801Cxxxx) are "
               "counted in the port stub total but not the matching-build "
               "universe.")
    out.append("- The matchings/nonmatchings **directory split is a lagging "
               "indicator** and is deliberately NOT used for status -- only the "
               "union (as the denominator). Re-run after any resync.")

    text = "\n".join(out) + "\n"
    sys.stdout.write(text)

    if args.json:
        n_stubs_total = len(stub_funcs)
        payload = {
            "universe_total": tot["total"],
            "totals": dict(tot),
            "matched_inferred": tot["matched"],
            "matchings_dir_count": matchings_dir_count(root),
            "per_overlay": {ov: dict(c) for ov, c in ov_tot.items()},
            "per_tu": [{"overlay": ov, "module": mod, **c}
                       for (ov, mod), c in sorted(rows.items())],
            "port": {
                "real": port_real,
                "stubbed_total": n_stubs_total,
                "stubbed_in_universe": len(stub_funcs & universe_funcs),
                "stubbed_out_of_universe": n_stubs_total - len(stub_funcs & universe_funcs),
                "excluded_psyq": port_excl,
            },
        }
        with open(args.json, "w") as fh:
            json.dump(payload, fh, indent=2)
        sys.stderr.write(f"\n[decomp_status] JSON written to {args.json}\n")


if __name__ == "__main__":
    main()
