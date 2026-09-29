#!/usr/bin/env python3
"""Psy-Q exclusion guard for the native PC port (pc_port/).

Owner policy (2026-09-28): the port contains no Sony Psy-Q SDK code -- no
linked SDK objects, no decompiled / translated Psy-Q C, no Psy-Q data tables.
Its PSX layer is PsyCross (MIT, pc_port/THIRD_PARTY.md) plus the port's own
compatibility layer (pc_port/src/psyq_compat.c and the other files in
COMPAT_FILES below).  The MATCHING build keeps its Psy-Q TUs
(src/slus_006.64/psyq/**) for byte-exactness; this guard only looks at the
port.  Adapted from the Parasite Eve decomp's tools/analysis/psyq_port_guard.py.

Checks (classification: tools/analysis/psyq_classify.py over
config/slus_006.64.yaml + config/psyq_data_ranges.tsv)
  func    -- a pc_port source DEFINES func_XXXXXXXX at a Psy-Q address, or
             carries a `decomp-source: func_XXXXXXXX` header for one.  Test
             doubles (mock callees under pc_port/tests) are not definitions
             of SDK code and only count via a decomp-source header.
  name    -- a runtime source (pc_port/src/**) outside COMPAT_FILES defines a
             function with the name of a Psy-Q SDK function (a re-implementation
             that belongs in the compat layer, written from documented
             behaviour).  Test doubles under pc_port/tests are not checked.
  include -- a pc_port source #includes a matching-build Psy-Q TU
             (src/**/psyq/**.c).
  tu      -- (--binary) the linked executable's DWARF names a compile unit
             under src/**/psyq/ : a Psy-Q TU was linked into the port.
  bfunc   -- (--binary) the executable defines func_XXXXXXXX at a Psy-Q address.
  table   -- Psy-Q data tables (address ranges in config/psyq_data_ranges.tsv).
             With the user's disc/SLUS_006.64 the table bytes are read from
             THE USER'S DISC and looked for as numeric-literal runs in
             pc_port sources and as raw bytes in every --binary.  Without the
             disc the table check is skipped (warning) unless --require-exe.

Allowlist (pc_port/psyq_guard_allowlist.txt) holds known offenders while the
replacement lands.  It must SHRINK: a finding not in it fails, an entry no
longer found fails (stale).  --strict ignores it (goal state).

Exit 0 = clean, 1 = findings, 2 = usage error.
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import psyq_classify  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
DEF_ALLOW = ROOT / "pc_port/psyq_guard_allowlist.txt"
DEF_EXE = ROOT / "disc/SLUS_006.64"
EXE_SHA256 = "dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
TEXT_VRAM_BIAS = 0x80010000 - 0x800
SRC_EXT = (".c", ".cc", ".cpp", ".C", ".h", ".hpp", ".inc", ".inl")
SKIP_DIRS = {"extern", ".git", "__pycache__"}
# The port's Psy-Q API compatibility layer: clean-room implementations of the
# SDK's documented interface on top of PsyCross.  Defining SDK names is their
# job, so the `name` check does not apply to them (func/table checks do).
# Their func_XXXXXXXX definitions at Psy-Q addresses (SDK entry points the
# game calls by address) are likewise part of the compat layer.
COMPAT_FILES = {
    "pc_port/src/psyq_compat.c",
    "pc_port/src/psyq_cd_mix.c",
    "pc_port/src/memcard_port.c",   # BIOS/libcard memory-card API on host files
    "pc_port/src/pc_file_io.c",     # libsn PCopen/PCread... on POSIX file I/O
    "pc_port/src/psyq_compat_normal_light_col.c",
    "pc_port/src/psyq_compat_spu_noise_clock.c",
    "pc_port/src/psyq_compat_leaf.inc",  # included by retail_leaf_adapters.c
    "pc_port/src/controller_vblank_service.c",  # libetc vblank callback on the host
}

FUNC_DEF_RE = re.compile(
    r"^[ \t]*(?:static[ \t]+|extern[ \t]+|inline[ \t]+|__attribute__\(\([^)]*\)\)[ \t]*)*"
    r"(?:[A-Za-z_][\w \t\*]*?)?\b([A-Za-z_]\w*)[ \t]*\(([^;{}()]*(?:\([^;{}()]*\)[^;{}()]*)*)\)[ \t\r\n]*"
    r"(?://[^\n]*\n[ \t\r\n]*|/\*.*?\*/[ \t\r\n]*)*\{",
    re.M | re.S,
)
COMPAT_ADDRS: set[int] = set()  # filled by check_sources
KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "do", "else"}
DECOMP_SRC_RE = re.compile(r"decomp-source:\s*(?:\S+/)?func_([0-9A-Fa-f]{8})")
INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.M)
NUM_RE = re.compile(r"(?<![\w.])(-?0[xX][0-9A-Fa-f]+|-?\d+)[uUlL]*(?![\w.])")


def iter_sources(tree: Path):
    for root, dirs, files in os.walk(tree):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS and not d.startswith("build")]
        for fn in files:
            if fn.endswith(SRC_EXT):
                yield Path(root) / fn


def rel(p: Path) -> str:
    try:
        return str(p.resolve().relative_to(ROOT))
    except ValueError:
        return str(p)


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: re.sub(r"[^\n]", " ", m.group(0)), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


# --------------------------------------------------------------------------- checks
def check_sources(tree: Path, cls: psyq_classify.Classification):
    out = []
    for p in iter_sources(tree):
        try:
            raw = p.read_text(errors="replace")
        except OSError:
            continue
        r = rel(p)
        text = strip_comments(raw)
        seen = set()
        for m in FUNC_DEF_RE.finditer(text):
            name = m.group(1)
            if name in KEYWORDS:
                continue
            fm = re.fullmatch(r"func_([0-9A-Fa-f]{8})", name)
            if fm:
                a = int(fm.group(1), 16)
                lib = cls.library(a)
                if lib and a not in seen and not r.startswith("pc_port/tests/"):
                    seen.add(a)
                    if r in COMPAT_FILES:
                        COMPAT_ADDRS.add(a)
                    else:
                        out.append(("func", f"0x{a:08X}", r, f"{lib} defined"))
            elif (name in cls.names and r.startswith("pc_port/src/") and r not in COMPAT_FILES
                  and name not in seen):
                seen.add(name)
                out.append(("name", name, r, f"{cls.names[name][1]} SDK function re-implemented outside the compat layer"))
        for m in DECOMP_SRC_RE.finditer(raw):
            a = int(m.group(1), 16)
            lib = cls.library(a)
            if lib and a not in seen:
                seen.add(a)
                out.append(("func", f"0x{a:08X}", r, f"{lib} decomp-source"))
        for m in INCLUDE_RE.finditer(text):
            inc = m.group(1)
            if re.search(r"(^|/)psyq/[^\"]*\.(c|s|inc)$", inc):
                out.append(("include", inc, r, "matching-build Psy-Q TU"))
    return out


def tool(*names):
    for n in names:
        t = shutil.which(n)
        if t:
            return t
    raise SystemExit(f"psyq_port_guard: need one of {names} for --binary")


def check_binary(binary: Path, cls):
    out = []
    readelf = tool("readelf", "llvm-readelf")
    r = subprocess.run([readelf, "--debug-dump=info", "--dwarf-depth=1", str(binary)],
                       capture_output=True, text=True, errors="replace")
    cus = set()
    for line in r.stdout.splitlines():
        m = re.search(r"DW_AT_name\s*:\s*(?:\(indirect[^)]*\):\s*)?(.+)$", line)
        if m:
            cus.add(m.group(1).strip())
    if not cus:
        print(f"psyq_port_guard: WARNING {binary.name} has no DWARF; TU check skipped "
              "(build with -g)", file=sys.stderr)
    for cu in sorted(cus):
        if re.search(r"(^|/)src/[^ ]*/psyq/", cu) and "extern/PsyCross" not in cu:
            out.append(("tu", cu.split("xenogears-decomp/")[-1], f"bin:{binary.name}", "Psy-Q TU linked"))
    nm = tool("nm", "llvm-nm")
    r = subprocess.run([nm, "--defined-only", str(binary)], capture_output=True, text=True)
    for line in r.stdout.splitlines():
        m = re.search(r"\s[TtWw]\s+_?func_([0-9A-Fa-f]{8})\b", line)
        if m:
            a = int(m.group(1), 16)
            lib = cls.library(a)
            if lib and a not in COMPAT_ADDRS:
                out.append(("bfunc", f"0x{a:08X}", f"bin:{binary.name}", f"{lib} linked"))
    return out


def psycross_symbols(binary: Path) -> set[str]:
    """Data symbols defined by libpsycross.a (PsyCross, MIT, pinned in
    pc_port/psycross.lock).  PsyCross upstream ships its own GTE tables
    (rcossin_tbl, SQRT); those are the accepted third-party layer."""
    libs = sorted(ROOT.glob("pc_port/build*/**/libpsycross.a"))
    if not libs:
        return set()
    nm = tool("nm", "llvm-nm")
    r = subprocess.run([nm, "--defined-only", str(libs[0])], capture_output=True, text=True)
    return {f[2] for f in (l.split() for l in r.stdout.splitlines()) if len(f) == 3}


def symbol_map(binary: Path):
    """[(file offset lo, hi, name)] of sized symbols in file-backed sections."""
    readelf = tool("readelf", "llvm-readelf")
    secs = []
    for l in subprocess.run([readelf, "-SW", str(binary)], capture_output=True, text=True).stdout.splitlines():
        m = re.match(r"\s*\[\s*\d+\]\s+(\S+)\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)", l)
        if m and m.group(2) != "NOBITS":
            secs.append((int(m.group(3), 16), int(m.group(4), 16), int(m.group(5), 16)))
    nm = tool("nm", "llvm-nm")
    out = []
    for l in subprocess.run([nm, "-S", "--defined-only", str(binary)], capture_output=True, text=True).stdout.splitlines():
        f = l.split()
        if len(f) != 4:
            continue
        va, sz = int(f[0], 16), int(f[1], 16)
        for addr, off, size in secs:
            if addr and addr <= va < addr + size:
                out.append((off + va - addr, off + va - addr + sz, f[3]))
                break
    return out


def check_tables_binary(binary: Path, tables):
    blob = binary.read_bytes()
    exempt = psycross_symbols(binary)
    syms = symbol_map(binary) if exempt else []
    out = []
    for name, data in tables:
        win = 32
        hits, owners = 0, set()
        for i in range(0, max(len(data) - win, 0) + 1, win):
            j = blob.find(data[i:i + win])
            while j >= 0:
                owner = next((n for lo, hi, n in syms if lo <= j < hi), None)
                if owner not in exempt:
                    hits += 1
                    owners.add(owner or "?")
                    break
                j = blob.find(data[i:i + win], j + 1)
        if hits >= 2:
            out.append(("table", name, f"bin:{binary.name}",
                        f"{hits} x32B windows of the table bytes in {', '.join(sorted(owners))}"))
    return out


def check_tables_sources(tree: Path, tables, run_len: int = 24, distinct_words: int = 24):
    out = []
    prepared = []
    for name, data in tables:
        u16 = [int.from_bytes(data[i:i + 2], "little") for i in range(0, len(data) - 1, 2)]
        u32 = {int.from_bytes(data[i:i + 4], "little") for i in range(0, len(data) - 3, 4)}
        u32 = {w for w in u32 if w >= 0x10000}
        starts = {tuple(u16[i:i + run_len]) for i in range(0, len(u16) - run_len + 1)
                  if len(set(u16[i:i + run_len])) > 4}
        prepared.append((name, starts, u32))
    for p in iter_sources(tree):
        try:
            text = p.read_text(errors="replace")
        except OSError:
            continue
        nums = []
        for m in NUM_RE.finditer(text):
            try:
                nums.append(int(m.group(1), 0) & 0xFFFFFFFF)
            except ValueError:
                pass
        if len(nums) < run_len:
            continue
        numset = set(nums)
        n16 = [v & 0xFFFF for v in nums]
        for name, starts, u32 in prepared:
            hit = None
            common = len(u32 & numset)
            if common >= distinct_words:
                hit = f"{common} distinct table words as literals"
            else:
                for i in range(0, len(n16) - run_len + 1):
                    if tuple(n16[i:i + run_len]) in starts:
                        hit = f"run of >= {run_len} consecutive table values"
                        break
            if hit:
                out.append(("table", name, rel(p), hit))
    return out


def load_allow(path: Path) -> set[tuple[str, str, str]]:
    out = set()
    if not path.exists():
        return out
    for line in path.read_text().splitlines():
        s = line.split("#", 1)[0].rstrip()
        if not s.strip():
            continue
        f = s.split("\t")
        if len(f) >= 3:
            out.add((f[0], f[1], f[2]))
    return out


# --------------------------------------------------------------------------- main
def run(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--tree", default=str(ROOT / "pc_port"))
    ap.add_argument("--allowlist", default=str(DEF_ALLOW))
    ap.add_argument("--binary", action="append", default=[])
    ap.add_argument("--exe", default=str(DEF_EXE), help="retail SLUS_006.64 (tables)")
    ap.add_argument("--no-disc", action="store_true", help="skip the table check")
    ap.add_argument("--require-exe", action="store_true")
    ap.add_argument("--no-sources", action="store_true")
    ap.add_argument("--strict", action="store_true", help="ignore the allowlist (goal state)")
    ap.add_argument("--write-allowlist", action="store_true")
    ap.add_argument("-q", "--quiet", action="store_true")
    a = ap.parse_args(argv)

    cls = psyq_classify.Classification()
    if not cls.ranges:
        print("psyq_port_guard: empty classification", file=sys.stderr)
        return 2
    tree = Path(a.tree)
    findings = []
    compat_scan = check_sources(tree, cls)  # also fills COMPAT_ADDRS
    if not a.no_sources:
        findings += compat_scan
    for b in a.binary:
        if not Path(b).is_file():
            print(f"psyq_port_guard: --binary {b} does not exist", file=sys.stderr)
            return 2
        findings += check_binary(Path(b), cls)

    tables = []
    exe_path = Path(a.exe)
    if not a.no_disc and exe_path.is_file():
        import hashlib
        exe = exe_path.read_bytes()
        if hashlib.sha256(exe).hexdigest() != EXE_SHA256:
            print(f"psyq_port_guard: {exe_path} is not the retail USA SLUS_006.64; "
                  "table check skipped", file=sys.stderr)
        else:
            for name, lo, hi, _lib in psyq_classify.load_data_ranges():
                tables.append((name, exe[lo - TEXT_VRAM_BIAS:hi - TEXT_VRAM_BIAS]))
    elif a.require_exe:
        print(f"psyq_port_guard: --require-exe but {exe_path} is missing", file=sys.stderr)
        return 2
    elif not a.quiet or not a.no_disc:
        print("psyq_port_guard: WARNING table check skipped "
              f"({'--no-disc' if a.no_disc else 'no disc/SLUS_006.64'})", file=sys.stderr)
    if tables:
        if not a.no_sources:
            findings += check_tables_sources(tree, tables)
        for b in a.binary:
            findings += check_tables_binary(Path(b), tables)

    findings = sorted(set(findings))
    keys = {(k, key, where) for k, key, where, _ in findings}
    if a.write_allowlist:
        with open(a.allowlist, "w") as fh:
            fh.write("# Psy-Q port guard allowlist -- known offenders pending replacement.\n"
                     "# This file must only SHRINK; the goal state is an empty file and\n"
                     "# `tools/analysis/psyq_port_guard.py --strict` passing.\n"
                     "# kind<TAB>key<TAB>where\n")
            for k in sorted(keys):
                fh.write("\t".join(k) + "\n")
        print(f"psyq_port_guard: wrote {len(keys)} allowlist entries to {a.allowlist}")
        return 0

    allow = set() if a.strict else load_allow(Path(a.allowlist))
    new = [f for f in findings if (f[0], f[1], f[2]) not in allow]
    ran_bins = {f"bin:{Path(b).name}" for b in a.binary}

    def ran(entry):
        k, _key, where = entry
        if where.startswith("bin:"):
            return where in ran_bins and (k != "table" or bool(tables))
        if a.no_sources:
            return False
        return k != "table" or bool(tables)

    stale = sorted(e for e in allow if ran(e) and e not in keys)
    if not a.quiet:
        print(f"psyq_port_guard: {len(findings)} Psy-Q finding(s) "
              f"({'strict' if a.strict else f'{len(allow)} allowlisted'})")
    for f in new[:200]:
        print("NEW   " + "\t".join(f))
    for e in stale:
        print("STALE " + "\t".join(e) + "   (no longer found: delete it from the allowlist)")
    if new or stale:
        print(f"psyq_port_guard: FAIL ({len(new)} new, {len(stale)} stale)")
        return 1
    print("psyq_port_guard: PASS" + (" (strict: no Psy-Q code or tables)" if a.strict or not findings
                                     else f" (relative to allowlist; {len(findings)} offenders remain)"))
    return 0


if __name__ == "__main__":
    sys.exit(run())
