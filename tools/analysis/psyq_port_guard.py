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
  provider -- a reviewed host SDK implementation has a different identity or location.
  include -- a pc_port source #includes a matching-build Psy-Q TU
             (src/**/psyq/**.c).
  tu      -- (--binary) the linked executable's DWARF names a compile unit
             under src/**/psyq/ : a Psy-Q TU was linked into the port.
  bfunc   -- (--binary) the executable defines func_XXXXXXXX at a Psy-Q address.
  nodwarf -- (--binary --require-dwarf) translation-unit evidence is missing.
  path    -- (--binary) an absolute build-machine path (/home/, /tmp/, /root/,
             /xenogears-decomp/) in a loaded section of the executable
             (e.g. an assert's __FILE__): personal infrastructure must not ship.
  table   -- Psy-Q data tables (address ranges in config/psyq_data_ranges.tsv).
             With the user's disc/SLUS_006.64 the table bytes are read from
             THE USER'S DISC and looked for as numeric-literal runs in
             pc_port sources and as raw bytes in every --binary.  Without the
             disc the table check is skipped (warning) unless --require-exe.

Allowlist (pc_port/psyq_guard_allowlist.txt) holds known offenders while the
replacement lands.  It must SHRINK: a finding not in it fails, an entry no
longer found fails (stale). --strict ignores exemptions, including PsyCross's
legacy table exemption. --require-empty-allowlist also rejects any active
allowlist entry; release packaging requires both flags.

Exit 0 = clean, 1 = findings, 2 = usage error.
"""
from __future__ import annotations

import argparse
import hashlib
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
# Compatibility translation units supply SDK entry points to game code.
# For entries in REVIEWED_PROVIDERS the implementation's individual source
# identity takes precedence over this translation-unit classification.
COMPAT_FILES = {
    "pc_port/src/psyq_compat.c",
    "pc_port/src/psyq_cd_mix.c",
    "pc_port/src/memcard_port.c",   # BIOS/libcard memory-card API on host files
    "pc_port/src/pc_file_io.c",     # libsn PCopen/PCread... on POSIX file I/O
    "pc_port/src/psyq_compat_normal_light_col.c",
    "pc_port/src/psyq_compat_transpose_matrix.c",
    "pc_port/src/psyq_compat_spu_noise_clock.c",
    "pc_port/src/psyq_compat_leaf.inc",  # included by retail_leaf_adapters.c
    "pc_port/src/controller_vblank_service.c",  # libetc vblank callback on the host
}

# Source identities for reviewed host implementations. Hash the whole file so
# helpers, macros, includes, and preprocessor conditions are covered as well.
# A name moved back into a legacy compatibility TU must be reviewed again.
REVIEWED_PROVIDERS = {
    "pc_port/src/cleanroom/host_vsync.inc": (
        "3d27bf27cfffaf876f2669424686b2482f6e7bbf32c70a2d5c9479361fccfe33", ("Vsync",)),
    "pc_port/src/cleanroom/sdk_card_lifecycle.inc": (
        "b5e25ae020e8b92b13badf8e4442a86e9e9ea2393bdf8dddd06411113e566f94", ('InitCARD', 'StartCARD', 'StopCARD')),
    "pc_port/src/cleanroom/sdk_cd_audio_matrix.inc": (
        "e42ca113783d4737c716f4f0fcef97c9e7f059db60238565bef3dce78824db30", ('CdMix',)),
    "pc_port/src/cleanroom/sdk_cd_transfer_status.inc": (
        "f5ad825854c13cf9308ebfbc5fc9ab2a8313c9fdd9570820355d7c71d15ffb31", ('CdDataSync',)),
    "pc_port/src/cleanroom/sdk_file_transfers.inc": (
        "aa9fe4e93311a9fd186c87913d2c9dad8e8633802fe2ee4ce0b916269816325a", ('PCread', 'PCwrite')),
    "pc_port/src/cleanroom/sdk_libc_rand.inc": (
        "3ab5d1af4a5a09fa53fd7c82cde57c61bc302d6fd3be9981ab00bdcdf085adbb", ('rand',)),
    "pc_port/src/cleanroom/sdk_libc_sprintf.inc": (
        "3453ab72c60d6fa41088f4408cc475d4a66195a391d100d29c400ac26abf3ded", ('Sprintf',)),
    "pc_port/src/cleanroom/sdk_normal_light_col.inc": (
        "b535a4dc8ef6e7c83a8b3b5dfa5b4a01ea76f59d9e2b152f6cc841788a10cddb", ('NormalLightCol',)),
    "pc_port/src/cleanroom/sdk_packet_SetLineF2.inc": (
        "ef30fb55ec87967387b577840527ee672df64f734d5ca902749b9a9b23665fe1", ('SetLineF2',)),
    "pc_port/src/cleanroom/sdk_packet_SetLineG2.inc": (
        "fc6a50ef2607e8856b05600fa554433a0e449ad33e4498f9d537d2fcc332b8d9", ('SetLineG2',)),
    "pc_port/src/cleanroom/sdk_packet_SetPolyF3.inc": (
        "1aa0538cdd51d735dd08f9bb2b8ff1656c43bb20132314934fad3227bd693cc4", ('SetPolyF3',)),
    "pc_port/src/cleanroom/sdk_packet_SetPolyFT3.inc": (
        "3027b1f454d56d49611c5391916e044ab8b99aea5829883d9ef77620ff96164f", ('SetPolyFT3',)),
    "pc_port/src/cleanroom/sdk_packet_SetPolyG3.inc": (
        "a652624baef610818d1524d4cf1c69d964ca7de44c2b08f32d41860fe0aa2263", ('SetPolyG3',)),
    "pc_port/src/cleanroom/sdk_packet_SetPolyGT3.inc": (
        "70c294f5e04666d5d90daadd688c21407cc57e341b515b4bd69a131e559c4ee2", ('SetPolyGT3',)),
    "pc_port/src/cleanroom/sdk_pad_shutdown.inc": (
        "c9d445f70d23025a957a3d2b629a46f36c83c9deeb2bc9a291109370f0557b16", ('func_800408F4',)),
    "pc_port/src/cleanroom/sdk_pad_vblank_registration.inc": (
        "5457fea35b49f5d2175159f3f102b2f90c429cbc948f9e08a1c4582e53df2423", ('func_8004B7D0',)),
    "pc_port/src/cleanroom/sdk_read_geom_offset.inc": (
        "61cd3d4824cc5e327cb8f406b447f4f470b825dd99f1f08963daf275a34e0513", ('ReadGeomOffset',)),
    "pc_port/src/cleanroom/sdk_read_geom_screen.inc": (
        "3ca87fc1a2766e5ecb991ccfb65c0ec74a812e88698ab74df60d486708e40ad0", ('ReadGeomScreen',)),
    "pc_port/src/cleanroom/sdk_rot_average4.inc": (
        "7e45eb90fd25ec7a4ecfdbb2380230ab959ec6d0dfed13672d5e4ad2e6a19715", ('RotAverage4',)),
    "pc_port/src/cleanroom/sdk_spu_noise_clock.inc": (
        "7fbf507b9839e0ad527f3bd38297243d2e0116aad980d94787d265daaefee2da", ('SpuSetNoiseClock',)),
    "pc_port/src/cleanroom/sdk_vector_products.inc": (
        "a3403652acc7c47bac804fac59d417a4708302e18419588dc04b0c79c9002c53", ('OuterProduct0', 'OuterProduct12', 'Square0')),
    "pc_port/src/psyq_compat_transpose_matrix.c": (
        "ef3d1313b5c9d6acd8736b5d7bcfc39113a894b8d54ee49556c19b0e85aaf0f3", ('TransposeMatrix',)),
}
REVIEWED_SYMBOLS = {name for _, names in REVIEWED_PROVIDERS.values() for name in names}
# These public SDK entries are provided by the qualified PsyCross build.
VENDOR_SYMBOLS = {"ClearImage", "CdFlush"}
QUALIFIED_SYMBOLS = REVIEWED_SYMBOLS | VENDOR_SYMBOLS

# A reviewed fragment by itself cannot authorize an address in a linked binary.
PAD_PROVIDER_INCLUDES = {
    "pc_port/src/psyq_compat.c": ("cleanroom/sdk_pad_shutdown.inc", 0x800408F4),
    "pc_port/src/controller_vblank_service.c": ("cleanroom/sdk_pad_vblank_registration.inc", 0x8004B7D0),
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


def rel(p: Path, tree: Path) -> str:
    # An external pc_port checkout has the same project-relative source IDs.
    # Compare complete paths below the selected root, never filename suffixes.
    base = tree.parent if tree.name == "pc_port" else ROOT
    try:
        return p.resolve().relative_to(base).as_posix()
    except ValueError:
        return str(p.resolve())


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: re.sub(r"[^\n]", " ", m.group(0)), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


# --------------------------------------------------------------------------- checks
def check_sources(tree: Path, cls: psyq_classify.Classification):
    tree = tree.resolve()
    out = []
    present = set()
    reviewed_addresses = set()
    included_addresses = set()
    COMPAT_ADDRS.clear()
    for p in iter_sources(tree):
        r = rel(p, tree)
        # Use both the spelling in the source tree and its resolved location:
        # a symlink under an approved name must not qualify another file.
        lexical = p.relative_to(tree.parent).as_posix() if tree.name == "pc_port" else r
        legacy_compat = r == lexical and r in COMPAT_FILES
        expected = REVIEWED_PROVIDERS.get(lexical)
        try:
            source_bytes = p.read_bytes()
            raw = source_bytes.decode("utf-8", errors="replace")
        except OSError:
            if expected:
                out.append(("provider", "unreadable", lexical, "cannot verify SDK source identity"))
            continue
        reviewed_names = ()
        if expected:
            present.add(lexical)
            expected_hash, names = expected
            if r == lexical and hashlib.sha256(source_bytes).hexdigest() == expected_hash:
                reviewed_names = names
            else:
                out.append(("provider", "sha256", lexical, "reviewed SDK source identity differs"))
        text = strip_comments(raw)
        seen = set()
        for m in FUNC_DEF_RE.finditer(text):
            name = m.group(1)
            if name in KEYWORDS:
                continue
            fm = re.fullmatch(r"func_([0-9A-Fa-f]{8})", name)
            sdk_name = f"func_{fm.group(1).upper()}" if fm else name
            if lexical.startswith("pc_port/src/") and sdk_name in QUALIFIED_SYMBOLS:
                if name not in reviewed_names:
                    out.append(("provider", name, lexical, "unverified SDK provider"))
                    continue
                if fm:
                    reviewed_addresses.add(int(fm.group(1), 16))
                continue
            if fm:
                a = int(fm.group(1), 16)
                lib = cls.library(a)
                if lib and a not in seen and not r.startswith("pc_port/tests/"):
                    seen.add(a)
                    if legacy_compat:
                        COMPAT_ADDRS.add(a)
                    else:
                        out.append(("func", f"0x{a:08X}", r, f"{lib} defined"))
            elif (name in cls.names and lexical.startswith("pc_port/src/") and not legacy_compat
                  and name not in seen):
                seen.add(name)
                out.append(("name", name, r, f"{cls.names[name][1]} SDK function re-implemented outside the compat layer"))
        for m in DECOMP_SRC_RE.finditer(raw):
            a = int(m.group(1), 16)
            lib = cls.library(a)
            if lib and a not in seen:
                seen.add(a)
                out.append(("func", f"0x{a:08X}", r, f"{lib} decomp-source"))
        pad_provider = PAD_PROVIDER_INCLUDES.get(lexical) if r == lexical else None
        for m in INCLUDE_RE.finditer(text):
            inc = m.group(1).replace("\\", "/")
            if pad_provider and inc == pad_provider[0]:
                included_addresses.add(pad_provider[1])
            if re.search(r"(^|/)psyq/[^\"]*\.(c|s|inc)$", inc):
                out.append(("include", inc, r, "matching-build Psy-Q TU"))
    COMPAT_ADDRS.update(reviewed_addresses & included_addresses)
    if tree == (ROOT / "pc_port").resolve():
        for missing in sorted(REVIEWED_PROVIDERS.keys() - present):
            out.append(("provider", "missing", missing, "reviewed SDK source is absent"))
        for owner, (include, address) in PAD_PROVIDER_INCLUDES.items():
            if address not in included_addresses:
                out.append(("provider", include, owner, "SDK entry include is absent"))
    return out


def tool(*names):
    for n in names:
        t = shutil.which(n)
        if t:
            return t
    raise SystemExit(f"psyq_port_guard: need one of {names} for --binary")


def binary_tool(args):
    """A failed inspection must never become an empty, passing scan."""
    result = subprocess.run(args, capture_output=True, text=True, errors="replace")
    if result.returncode:
        print(f"psyq_port_guard: inspection failed: {args[0]}: {result.stderr.strip()}",
              file=sys.stderr)
        raise SystemExit(2)
    return result.stdout


def check_binary(binary: Path, cls, require_dwarf: bool = False):
    out = []
    readelf = tool("readelf", "llvm-readelf")
    dwarf = binary_tool([readelf, "--debug-dump=info", "--dwarf-depth=1", str(binary)])
    cus = set()
    for line in dwarf.splitlines():
        m = re.search(r"DW_AT_name\s*:\s*(?:\(indirect[^)]*\):\s*)?(.+)$", line)
        if m:
            cus.add(m.group(1).strip())
    if not cus:
        if require_dwarf:
            out.append(("nodwarf", binary.name, f"bin:{binary.name}",
                        "no DWARF: the TU check cannot run (scan the unstripped build binary)"))
        else:
            print(f"psyq_port_guard: WARNING {binary.name} has no DWARF; TU check skipped "
                  "(build with -g)", file=sys.stderr)
    for cu in sorted(cus):
        if re.search(r"(^|/)src/[^ ]*/psyq/", cu) and "extern/PsyCross" not in cu:
            out.append(("tu", cu.split("xenogears-decomp/")[-1], f"bin:{binary.name}", "Psy-Q TU linked"))
    nm = tool("nm", "llvm-nm")
    for line in binary_tool([nm, "--defined-only", str(binary)]).splitlines():
        m = re.search(r"\s[TtWw]\s+_?func_([0-9A-Fa-f]{8})\b", line)
        if m:
            a = int(m.group(1), 16)
            lib = cls.library(a)
            if lib and a not in COMPAT_ADDRS:
                out.append(("bfunc", f"0x{a:08X}", f"bin:{binary.name}", f"{lib} linked"))
    return out


ABS_PATH_RE = re.compile(rb"/(?:home|tmp|root|xenogears-decomp)/[\x21-\x7e]{3,}")


def check_paths_binary(binary: Path):
    """Absolute build-machine paths in SHF_ALLOC sections (debug sections are
    stripped from packages and are not scanned)."""
    readelf = tool("readelf", "llvm-readelf")
    blob = binary.read_bytes()
    found = set()
    for l in binary_tool([readelf, "-SW", str(binary)]).splitlines():
        m = re.match(r"\s*\[\s*\d+\]\s+(\S+)\s+(\S+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+([0-9a-f]+)\s+\S+\s+(\S*)", l)
        if not m or m.group(2) == "NOBITS" or "A" not in m.group(5):
            continue
        off, size = int(m.group(3), 16), int(m.group(4), 16)
        for hit in ABS_PATH_RE.finditer(blob, off, off + size):
            found.add(hit.group(0).decode("ascii", "replace"))
    return [("path", p, f"bin:{binary.name}", "absolute build path in a loaded section") for p in sorted(found)]


def psycross_symbols(binary: Path) -> set[str]:
    """Data symbols defined by libpsycross.a (PsyCross, MIT, pinned in
    pc_port/psycross.lock).  PsyCross upstream ships its own GTE tables
    (rcossin_tbl, SQRT). This legacy exemption is disabled in strict mode;
    it must never be used by a release gate."""
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


def check_tables_binary(binary: Path, tables, strict: bool = False):
    """In --strict mode nothing is exempt: retail-identical SDK tables must not
    ship even inside PsyCross (they load from the user's disc)."""
    blob = binary.read_bytes()
    exempt = set() if strict else psycross_symbols(binary)
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
    tree = tree.resolve()
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
                out.append(("table", name, rel(p, tree), hit))
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
    ap.add_argument("--require-dwarf", action="store_true",
                    help="a --binary without DWARF is a finding (gate on the unstripped build binary)")
    ap.add_argument("--no-sources", action="store_true")
    ap.add_argument("--strict", action="store_true", help="ignore the allowlist (goal state)")
    ap.add_argument("--require-empty-allowlist", action="store_true",
                    help="reject non-comment allowlist entries (required for releases)")
    ap.add_argument("--write-allowlist", action="store_true")
    ap.add_argument("-q", "--quiet", action="store_true")
    a = ap.parse_args(argv)
    if a.no_disc and a.require_exe:
        ap.error("--no-disc conflicts with --require-exe")
    if a.require_dwarf and not a.binary:
        ap.error("--require-dwarf needs --binary")
    if a.write_allowlist and (a.strict or a.require_empty_allowlist):
        ap.error("--write-allowlist conflicts with strict/empty-allowlist checks")
    if a.require_empty_allowlist:
        try:
            entries = [line for line in Path(a.allowlist).read_text().splitlines()
                       if line.strip() and not line.lstrip().startswith("#")]
        except OSError as exc:
            print(f"psyq_port_guard: cannot read allowlist: {exc}", file=sys.stderr)
            return 2
        if entries:
            print("psyq_port_guard: --require-empty-allowlist but active entries remain",
                  file=sys.stderr)
            return 1

    cls = psyq_classify.Classification()
    if not cls.ranges:
        print("psyq_port_guard: empty classification", file=sys.stderr)
        return 2
    tree = Path(a.tree)
    findings = []
    compat_scan = check_sources(tree, cls)  # also fills COMPAT_ADDRS
    findings += [row for row in compat_scan if not a.no_sources or row[0] == "provider"]
    for b in a.binary:
        if not Path(b).is_file():
            print(f"psyq_port_guard: --binary {b} does not exist", file=sys.stderr)
            return 2
        findings += check_binary(Path(b), cls, a.require_dwarf)
        findings += check_paths_binary(Path(b))

    tables = []
    exe_path = Path(a.exe)
    if not a.no_disc and exe_path.is_file():
        exe = exe_path.read_bytes()
        if hashlib.sha256(exe).hexdigest() != EXE_SHA256:
            print(f"psyq_port_guard: {exe_path} is not the retail USA SLUS_006.64; "
                  "table check skipped", file=sys.stderr)
            if a.require_exe:
                return 2
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
            findings += check_tables_binary(Path(b), tables, a.strict)

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
    table_status = "tables checked" if tables else "table scan SKIPPED"
    print("psyq_port_guard: PASS" + (f" (strict: no findings; {table_status})" if a.strict
          else f" (relative to allowlist; {len(findings)} findings; {table_status})"))
    return 0


if __name__ == "__main__":
    sys.exit(run())
