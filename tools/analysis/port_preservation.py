#!/usr/bin/env python3
"""Preservation report for the PC port: what the port runs vs the matched C.

Definitions (docs/ARCHITECTURE-PORT.md records them verbatim):

  matched C            the function is in objdiff's byte-exact set
                       (build/progress.json, fuzzy_match_percent == 100; the
                       tree passes rom-check) AND the body the port compiles
                       is that same source with no XENO_PC_PORT arm inside it.
  matched C with port seam
                       same, but the shared body has an inner
                       #if/#ifdef XENO_PC_PORT (pointer width, host seam).
  port-only C shadowing matched C
                       the port runs a different body for a byte-exact
                       function: a XENO_PC_PORT arm in the game TU, or a
                       definition in pc_port/ (including wm_XXXXXXXX for
                       world_map func_XXXXXXXX).
  port-only C (no retail match yet)
                       a port body for a retail function that has no
                       byte-exact C yet.
  shim/compat          port infrastructure that is not a retail function:
                       psyq_compat, xg_plat, harness, generated stubs.
  PsyCross             the pinned PsyCross library.

Usage:
  port_preservation.py shadows  --progress build/progress.json --objdir OBJ
  port_preservation.py coverage --progress build/progress.json --objdir OBJ \
        --binary BIN --map MAP COVERAGE.txt [COVERAGE.txt ...]
"""
import argparse
import bisect
import json
import os
import re
import subprocess
import sys
from collections import Counter, defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CATS = ["matched C", "matched C with port seam", "port-only C shadowing matched C",
        "port-only C (no retail match yet)", "shim/compat", "PsyCross"]


SDK = set()  # Psy-Q SDK functions (objdiff category "sdk"): the platform
             # layer (PsyCross + psyq_compat) replaces them by design.


def load_progress(path):
    """(matched, universe) of retail GAME functions; SDK names go to SDK."""
    data = json.load(open(path))
    matched, universe = set(), set()
    for unit in data["units"]:
        cats = unit.get("metadata", {}).get("progress_categories", [])
        for fn in unit.get("functions", []):
            if "sdk" in cats:
                SDK.add(fn["name"])
                continue
            universe.add(fn["name"])
            NAME_OVERLAYS.setdefault(fn["name"], set()).add(unit["name"].split("/")[0])
            if fn.get("fuzzy_match_percent") == 100.0:
                matched.add(fn["name"])
                MATCHED_IN.add((unit["name"].split("/")[0], fn["name"]))
    return matched, universe


# (overlay, name) pairs that are byte-exact: overlays share address ranges,
# so a func_XXXXXXXX name can exist in several overlays with different status.
MATCHED_IN = set()
NAME_OVERLAYS = {}  # name -> overlays that have a function of that name


def src_matched(src, name, matched):
    """Byte-exact status of `name` as compiled from game TU `src`."""
    parts = src.split("/")
    if len(parts) > 2 and parts[0] == "src":
        return (parts[1], name) in MATCHED_IN
    return name in matched


# --- game TU scan: which XENO_PC_PORT arm each function body lives in -------
DIRECTIVE = re.compile(r"^\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)")
FUNC_START = re.compile(r"^(?!\s)(?!(?:if|for|while|switch|return|else|do|extern|typedef)\b)"
                        r"[A-Za-z_][\w\s\*]*?\b([A-Za-z_]\w*)\s*\([^;{})]*,\s*$")
FUNC_HEAD = re.compile(r"^(?!\s)(?!(?:if|for|while|switch|return|else|do)\b)"
                       r"[A-Za-z_][\w\s\*]*?\b([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*(\{)?\s*$")


_TOK = re.compile(r"\s*(defined|\|\||&&|!=|==|>=|<=|[!()<>]|\w+)")


def eval_cond(kind, expr, port):
    """Three-valued truth (True / False / None = unknown) of a directive
    condition with XENO_PC_PORT defined (port=True) or not.  Other macros
    are unknown, so `defined(XENO_PC_PORT) || MENU_PART == 2` is True in
    the port build and unknown in the retail build."""
    if kind == "ifdef":
        return port if expr.strip() == "XENO_PC_PORT" else None
    if kind == "ifndef":
        return (not port) if expr.strip() == "XENO_PC_PORT" else None
    expr = re.sub(r"/\*.*?\*/|//.*", "", expr)
    toks = [t for t in _TOK.findall(expr) if t]
    pos = [0]

    def peek():
        return toks[pos[0]] if pos[0] < len(toks) else None

    def take():
        t = peek()
        pos[0] += 1
        return t

    def primary():
        t = take()
        if t == "!":
            v = primary()
            return None if v is None else (not v)
        if t == "(":
            v = orexpr()
            if peek() == ")":
                take()
            return v
        if t == "defined":
            paren = peek() == "("
            if paren:
                take()
            name = take()
            if paren and peek() == ")":
                take()
            return port if name == "XENO_PC_PORT" else None
        if t == "XENO_PC_PORT":
            return port
        if t is not None and t.isdigit():
            v = int(t) != 0
            # comparisons of literals / unknown macros stay unknown
            if peek() in ("==", "!=", "<", ">", "<=", ">="):
                take(); primary()
                return None
            return v
        if peek() in ("==", "!=", "<", ">", "<=", ">="):
            take(); primary()
        return None

    def andexpr():
        v = primary()
        while peek() == "&&":
            take()
            w = primary()
            v = False if (v is False or w is False) else (True if (v and w) else None)
        return v

    def orexpr():
        v = andexpr()
        while peek() == "||":
            take()
            w = andexpr()
            v = True if (v is True or w is True) else (False if (v is False and w is False) else None)
        return v

    try:
        return orexpr()
    except Exception:
        return None


def _and3(a, b):
    if a is False or b is False:
        return False
    if a is True and b is True:
        return True
    return None


def _not3(a):
    return None if a is None else (not a)


def scan_tu(path):
    """name -> list of (region, has_inner_port_directive) for C definitions.

    region is 'port' (only the port build compiles it), 'retail' (only the
    retail build), 'both' or 'dead'.  Each #if/#elif/#else branch is
    evaluated for both builds (eval_cond), nested frames are combined, and
    brace depth is tracked per arm."""
    defs = defaultdict(list)
    # frame: dict(port=[prior conds], retail=[...], cur_p, cur_r, depth0, first_end, cur0, xp)
    stack = []
    depth = 0
    cur = None  # [name, region, seam, line counts by region]
    pending = None
    head = None  # [name, header text, region] of a multi-line header
    try:
        lines = open(path, errors="replace").read().split("\n")
    except OSError:
        return defs

    def branch_vals(fr):
        p, r = fr["cur_p"], fr["cur_r"]
        for pp, rr in fr["prior"]:
            p, r = _and3(p, _not3(pp)), _and3(r, _not3(rr))
        return p, r

    def region():
        port_ok, retail_ok = True, True
        for fr in stack:
            p, r = branch_vals(fr)
            if p is False:
                port_ok = False
            if r is False:
                retail_ok = False
        if port_ok and retail_ok:
            return "both"
        if port_ok:
            return "port"
        if retail_ok:
            return "retail"
        return "dead"

    # join continuation lines of directives
    joined = []
    buf = ""
    for line in lines:
        if buf or DIRECTIVE.match(line):
            if line.rstrip().endswith("\\"):
                buf += line.rstrip()[:-1] + " "
                continue
            joined.append(buf + line)
            buf = ""
        else:
            joined.append(line)

    for line in joined:
        m = DIRECTIVE.match(line)
        if m:
            kind, expr = m.group(1), m.group(2)
            xp = "XENO_PC_PORT" in expr
            if kind in ("ifdef", "ifndef", "if"):
                stack.append(dict(prior=[], cur_p=eval_cond(kind, expr, True),
                                  cur_r=eval_cond(kind, expr, False), depth0=depth,
                                  first_end=None, cur0=cur, xp=xp))
                if cur and xp:
                    cur[2] = True
            elif kind in ("else", "elif") and stack:
                fr = stack[-1]
                if fr["first_end"] is None:
                    fr["first_end"] = (depth, cur)
                fr["prior"].append((fr["cur_p"], fr["cur_r"]))
                if kind == "else":
                    fr["cur_p"], fr["cur_r"] = True, True
                else:
                    fr["cur_p"], fr["cur_r"] = eval_cond("if", expr, True), eval_cond("if", expr, False)
                    fr["xp"] = fr["xp"] or xp
                depth = fr["depth0"]
                cur = fr["cur0"]
                if cur and fr["xp"]:
                    cur[2] = True
            elif kind == "endif" and stack:
                fr = stack.pop()
                if fr["first_end"] is not None:
                    depth, cur = fr["first_end"]
            continue
        code = re.sub(r"//.*|/\*.*?\*/|\"(\\.|[^\"])*\"|'(\\.|[^'])*'", "", line)
        if depth == 0 and cur is None:
            if head is not None:
                # continuation of a multi-line function header
                head[1] += " " + code.strip()
                if head[1].count("(") <= head[1].count(")"):
                    if head[1].rstrip().endswith(";"):
                        head = None
                        continue
                    pending, head = [head[0], head[2], False, {"port": 0, "retail": 0, "both": 0}], None
                    if not code.count("{"):
                        continue
                else:
                    continue
            else:
                fm = FUNC_HEAD.match(code)
                hm = FUNC_START.match(code) if not fm else None
                if fm and fm.group(1) not in ("INCLUDE_ASM", "INCLUDE_RODATA"):
                    pending = [fm.group(1), region(), False, {"port": 0, "retail": 0, "both": 0}]
                    if fm.group(3) is None and not code.count("{"):
                        continue
                elif hm and hm.group(1) not in ("INCLUDE_ASM", "INCLUDE_RODATA"):
                    head = [hm.group(1), code.strip(), region()]
                    continue
                elif pending is not None and code.strip() and not code.strip().startswith("{"):
                    pending = None
        opens, closes = code.count("{"), code.count("}")
        if depth == 0 and opens and pending is not None:
            cur, pending = pending, None
        elif cur is not None and code.strip(" \t{};") :
            r = region()
            if r in cur[3]:
                cur[3][r] += 1
        depth += opens - closes
        if depth <= 0:
            depth = 0
            if cur is not None and closes:
                n = cur[3]
                # A seam whose port arm spans the body: (almost) no statement
                # is shared by both builds, so the port runs its own body.
                bodyarm = (cur[1] == "both" and n["port"] > 0 and
                           n["both"] <= max(2, (n["port"] + n["retail"]) // 8))
                defs[cur[0]].append((cur[1], cur[2], bodyarm))
                cur = None
    return defs


def game_tu_scans():
    scans = {}
    for base, _, files in os.walk(os.path.join(ROOT, "src")):
        for f in files:
            if f.endswith(".c"):
                p = os.path.join(base, f)
                scans[os.path.relpath(p, ROOT)] = scan_tu(p)
    return scans


def src_class(name, scan, matched, universe, src=""):
    """Class of a function the port compiles from a game TU."""
    if name not in universe:
        return "shim/compat"  # port helper living in a game TU (PcPort_*, diag)
    entries = scan.get(name, [])
    port_side = [e for e in entries if e[0] in ("port", "both")]
    if not port_side:
        return None
    region, seam, bodyarm = port_side[0]
    if not src_matched(src, name, matched):
        return "port-only C (no retail match yet)"
    if region == "port" or bodyarm:
        return "port-only C shadowing matched C"
    return "matched C with port seam" if seam else "matched C"


def retail_name(name, universe):
    if name in universe:
        return name
    m = re.match(r"wm_([0-9A-Fa-f]{8})$", name)
    if m and ("func_" + m.group(1).upper()) in universe:
        return "func_" + m.group(1).upper()
    return None


PORT_FILE_OVERLAY = [("world_map", "world_map"), ("battle", "battle"), ("field", "field"),
                     ("menu", "menu"), ("shop", "shop_menu"), ("member", "member_change_menu")]


def port_overlay(name, src):
    """Overlay a pc_port body belongs to, or None when it cannot be told."""
    if name.startswith("wm_"):
        return "world_map"
    base = os.path.basename(src or "")
    for prefix, ovl in PORT_FILE_OVERLAY:
        if base.startswith(prefix):
            return ovl
    return None


def port_class(name, matched, universe, src=""):
    """Class of a pc_port body.  func_XXXXXXXX names repeat across overlays,
    so the byte-exact test uses the body's overlay (from its name or file);
    when that is unknown the name must be byte-exact in every overlay that
    has it, so a shadow is never claimed from another overlay's match."""
    r = retail_name(name, universe)
    if r is None:
        return "shim/compat", None
    ovl = port_overlay(name, src)
    if ovl is not None:
        if not NAME_OVERLAYS.get(r) or ovl in NAME_OVERLAYS[r]:
            is_matched = (ovl, r) in MATCHED_IN
        else:
            is_matched = False
    else:
        ovls = NAME_OVERLAYS.get(r, set())
        is_matched = bool(ovls) and all((o, r) in MATCHED_IN for o in ovls)
    if is_matched:
        return "port-only C shadowing matched C", r
    return "port-only C (no retail match yet)", r


_CU_CACHE = {}


def obj_to_src(objpath, objdir=None):
    """Source file of an object: its DWARF compile-unit name (built with -g)."""
    if objpath in _CU_CACHE:
        return _CU_CACHE[objpath]
    name = os.path.basename(objpath)
    res = subprocess.run(["readelf", "--debug-dump=info", "--dwarf-depth=1", objpath],
                         capture_output=True, text=True)
    m = re.search(r"DW_AT_name\s*:.*?:\s*(\S+)\s*$", res.stdout, re.M)
    if m:
        name = m.group(1)
        if name.startswith("/xenogears-decomp/"):
            name = name[len("/xenogears-decomp/"):]
    _CU_CACHE[objpath] = name
    return name


def defined_symbols(objdir):
    """[(object path, symbol)] for every text symbol in the build's objects."""
    out = []
    for base, _, files in os.walk(objdir):
        for f in files:
            if not f.endswith(".o"):
                continue
            p = os.path.join(base, f)
            res = subprocess.run(["nm", "--defined-only", p], capture_output=True, text=True)
            for line in res.stdout.splitlines():
                parts = line.split()
                if len(parts) == 3 and parts[1] in "Tt":
                    out.append((p, parts[2]))
    return out


def cmd_shadows(args):
    matched, universe = load_progress(args.progress)
    scans = game_tu_scans()
    rows = []
    # (1) XENO_PC_PORT arms in game TUs shadowing a byte-exact function
    for tu, scan in sorted(scans.items()):
        for name, entries in scan.items():
            if tu.startswith("src/slus_006.64/psyq/"):
                continue
            if src_matched(tu, name, matched) and any(e[0] == "port" for e in entries):
                rows.append((name, "port arm in " + tu, name))
            elif src_matched(tu, name, matched) and any(e[0] == "both" and e[2] for e in entries):
                rows.append((name, "port arm spanning the body in " + tu, name))
    # (2) pc_port definitions of byte-exact retail functions (incl. wm_)
    for obj, sym in defined_symbols(args.objdir):
        src = obj_to_src(obj, args.objdir)
        if src.startswith("src/"):
            continue
        cls, r = port_class(sym, matched, universe, src)
        if cls == "port-only C shadowing matched C":
            if src.endswith("stubs.c"):
                rows.append((r, "generated logging stub (matched C not linked)", sym))
            else:
                rows.append((r, "pc_port body " + sym + " (" + src + ")", sym))
    rows = sorted(set(rows))
    print("# Port bodies shadowing byte-exact matched C (objdiff 100%)")
    print("# retail function\twhere\tport symbol")
    for r in rows:
        print("\t".join(r))
    print(f"# total: {len(rows)} shadowing bodies for "
          f"{len(set(r[0] for r in rows))} matched functions", file=sys.stderr)


def load_map_ranges(mapfile):
    """sorted [(start, end, object)] for .text input sections."""
    ranges = []
    pend = None
    sec = re.compile(r"^ \.text(?:\.\S+)?\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)")
    sec1 = re.compile(r"^ \.text(?:\.\S+)?\s*$")
    cont = re.compile(r"^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+\.(?:o|a\(.*\)))\s*$")
    for line in open(mapfile, errors="replace"):
        m = sec.match(line)
        if m:
            s, n = int(m.group(1), 16), int(m.group(2), 16)
            if n:
                ranges.append((s, s + n, m.group(3)))
            pend = None
            continue
        if sec1.match(line):
            pend = True
            continue
        if pend:
            m = cont.match(line)
            if m:
                s, n = int(m.group(1), 16), int(m.group(2), 16)
                if n:
                    ranges.append((s, s + n, m.group(3)))
            pend = None
    ranges.sort()
    return ranges


def cmd_coverage(args):
    matched, universe = load_progress(args.progress)
    scans = game_tu_scans()
    syms = {}
    res = subprocess.run(["nm", "--defined-only", args.binary], capture_output=True, text=True)
    for line in res.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "Tt":
            syms.setdefault(int(parts[0], 16), parts[2])
    ranges = load_map_ranges(args.map)
    starts = [r[0] for r in ranges]
    objdir = os.path.abspath(args.objdir)
    for cov in args.coverage:
        funcs, calls = Counter(), Counter()
        detail = defaultdict(list)
        for line in open(cov):
            a, c = line.split()
            addr, n = int(a, 16), int(c)
            name = syms.get(addr, hex(addr))
            i = bisect.bisect_right(starts, addr) - 1
            obj = ranges[i][2] if i >= 0 and ranges[i][0] <= addr < ranges[i][1] else "?"
            if "libpsycross" in obj:
                cls = "PsyCross"
            else:
                objabs = obj if os.path.isabs(obj) else os.path.join(ROOT, obj)
                src = obj_to_src(objabs, objdir) if objabs.startswith(objdir) else obj
                cls = None
                if src.startswith("src/slus_006.64/psyq/"):
                    cls = "shim/compat"
                elif src.startswith("src/") and src in scans:
                    cls = src_class(name, scans[src], matched, universe, src)
                if cls is None:
                    cls = port_class(name, matched, universe, src)[0]
            funcs[cls] += 1
            calls[cls] += n
            detail[cls].append((n, name))
        tf, tc = sum(funcs.values()), sum(calls.values())
        game_f = sum(funcs[c] for c in CATS[:4])
        game_c = sum(calls[c] for c in CATS[:4])
        print(f"## {os.path.basename(cov)}")
        print("| class | functions | % | calls | % |")
        print("|---|---:|---:|---:|---:|")
        for c in CATS:
            print(f"| {c} | {funcs[c]} | {100*funcs[c]/max(tf,1):.1f} | {calls[c]} | {100*calls[c]/max(tc,1):.1f} |")
        print(f"| total | {tf} | | {tc} | |")
        mf = funcs[CATS[0]] + funcs[CATS[1]]
        mc = calls[CATS[0]] + calls[CATS[1]]
        print(f"\nmatched-C ratio of executed retail-function code: {mf}/{game_f} functions "
              f"({100*mf/max(game_f,1):.1f}%), {mc}/{game_c} calls ({100*mc/max(game_c,1):.1f}%)"
              f"  [strict matched C only: {funcs[CATS[0]]} functions]\n")
        if args.detail:
            with open(args.detail + "." + os.path.basename(cov) + ".tsv", "w") as f:
                for c in CATS:
                    for n, name in sorted(detail[c], reverse=True):
                        f.write(f"{c}\t{name}\t{n}\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("shadows")
    s.add_argument("--progress", required=True)
    s.add_argument("--objdir", required=True)
    c = sub.add_parser("coverage")
    c.add_argument("--progress", required=True)
    c.add_argument("--objdir", required=True)
    c.add_argument("--binary", required=True)
    c.add_argument("--map", required=True)
    c.add_argument("--detail")
    c.add_argument("coverage", nargs="+")
    args = ap.parse_args()
    (cmd_shadows if args.cmd == "shadows" else cmd_coverage)(args)


if __name__ == "__main__":
    main()
