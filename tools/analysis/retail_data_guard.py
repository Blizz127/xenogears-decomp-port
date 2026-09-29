#!/usr/bin/env python3
"""Retail-data guard: fail when retail Xenogears bytes/tables are committed.

Policy (owner, 2026-09-28): the repo, and in particular the native PC port
(pc_port/), ships no game data.  Everything from the game is read from the
user's own disc files at runtime (disc/, git-ignored).  Adapted from the
Parasite Eve decomp's tools/analysis/retail_data_guard.py.

What is scanned (tracked files, the staged index with --staged, or FILE...):
  * every numeric literal in a text file (hex ``0x..`` and decimal) as one
    token stream per file, encoded as little-endian u32 / u16 / u8 byte runs,
    plus the two stride-2 sub-streams in u32 (``{addr, value}`` pair tables);
  * every C string literal in a text file (adjacent literals joined);
  * the raw bytes of every binary (non-UTF-8) file, except host executables
    (ELF/PE: the committed MIPS toolchain) and files over 16 MiB.
A hit is any 32-byte window (>= 10 distinct byte values and not a near-
arithmetic sequence, so fill, counters and identity tables never trip it)
that equals retail bytes.

Reference data:
  1. the user's disc files in disc/ (SLUS_006.64 and the overlay .bin files
     listed in config/checksum.sha, checked against those retail hashes, plus
     the BIOS image scph5500.bin when present): every 4-aligned window;
  2. always: tools/analysis/retail_data_signatures.txt -- crc32 + truncated
     BLAKE2b of retail windows (one-way hashes, never bytes): a 64-byte-stride
     sample of every retail file (catches any copied run >= 96 bytes) and
     every 32-aligned window of the known ranges in
     tools/analysis/retail_data_known_ranges.tsv (tables that were once
     committed; addresses and sizes only).  Windows never overlap by more
     than they differ, so the hashes cannot be chained back into bytes.
     This is what CI uses (--no-disc).

Exit status: 0 clean, 1 hits, 2 usage/config error.

  retail_data_guard.py                       scan all tracked files
  retail_data_guard.py --staged              scan the staged index (pre-commit)
  retail_data_guard.py FILE...               scan specific files (working tree)
  retail_data_guard.py --no-disc             signatures only (CI behaviour)
  retail_data_guard.py --write-signatures    regenerate the signature file (disc)
  retail_data_guard.py --write-known-ranges  record current hits as known ranges
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import zlib
from array import array
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SIG_FILE = ROOT / "tools/analysis/retail_data_signatures.txt"
KNOWN_FILE = ROOT / "tools/analysis/retail_data_known_ranges.tsv"
ALLOW_FILE = ROOT / "tools/analysis/retail_data_guard_allow.txt"
CHECKSUMS = ROOT / "config/checksum.sha"
DISC = ROOT / "disc"

WIN = 32                # bytes per compared window
MIN_DISTINCT = 10       # distinct byte values required in a window
SIG_SAMPLE_STRIDE = 64  # offline sample stride over every retail file
SIG_KNOWN_STRIDE = 32   # offline stride over the known (once committed) ranges
MAX_BINARY = 16 << 20
EXE_HDR = 0x800

# Retail files: name in disc/, name in config/checksum.sha, guest vram base
# of file offset 0 (after the EXE header for SLUS), from config/<name>.yaml.
RETAIL_FILES = [
    ("SLUS_006.64", "slus_006.64", 0x80010000, EXE_HDR),
    ("field.bin", "field.bin", 0x8006FAF0, 0),
    ("battle.bin", "battle.bin", 0x8006FAF0, 0),
    ("battling.bin", "battling.bin", 0x8006FAF0, 0),
    ("world_map.bin", "world_map.bin", 0x8006FAF0, 0),
    ("movie.bin", "movie.bin", 0x8006FAF0, 0),
    ("menu.bin", "menu.bin", 0x801C5000, 0),
    ("shop_menu.bin", "shop_menu.bin", 0x801C5000, 0),
    ("member_change_menu.bin", "member_change_menu.bin", 0x801C5000, 0),
    ("battle_command_file1.bin", "battle_command_file1.bin", 0x801E5000, 0),
]
# SCPH-5500 BIOS (not game data, but Sony code: must not be committed either).
BIOS = ("scph5500.bin", "11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef")

TOKEN_RE = re.compile(rb"0[xX][0-9A-Fa-f]+|\b[0-9]+\b")
STRING_RE = re.compile(rb'"((?:[^"\\\n]|\\.)*)"')
JOIN_RE = re.compile(rb'"\s*"')
SKIP_SUFFIXES = {".png", ".jpg", ".jpeg", ".gif", ".ico", ".pdf", ".zip", ".gz"}


def eprint(*a):
    print(*a, file=sys.stderr)


_ARR = {4: "I", 2: "H", 1: "B"}


def window_ok(w: bytes, width: int = 4) -> bool:
    """Reject windows with little retail-specific information: few distinct
    byte values, or a near-arithmetic unit sequence (counters, identity)."""
    if len(set(w)) < MIN_DISTINCT:
        return False
    u = array(_ARR[width], w)
    deltas = {(u[i + 1] - u[i]) for i in range(len(u) - 1)}
    return len(deltas) >= 3


def address_only(w: bytes) -> bool:
    """Every u32 is a KSEG0 RAM or I/O address (or NULL): a pointer/jump table.  Those
    are the same facts as the committed symbol maps; reported separately
    ('address-table') and only failed under --strict."""
    u = array("I", w)
    addrs = sum(1 for x in u if 0x80000000 <= x < 0x80200000 or 0x1F800000 <= x < 0x1F802000)
    # NULL entries (terminators, empty slots) are allowed next to addresses
    return addrs >= 4 and all(x == 0 or 0x80000000 <= x < 0x80200000 or
                              0x1F800000 <= x < 0x1F802000 for x in u)


def blake8(w: bytes) -> str:
    return hashlib.blake2b(w, digest_size=8).hexdigest()


# -- reference index ---------------------------------------------------------

class Reference:
    def __init__(self):
        self.blobs: list[tuple[str, bytes, int]] = []  # label, data, vram of data[0]
        self.index: dict[int, list[tuple[int, int]]] = {}
        self.sigs: dict[int, set[str]] = {}
        self.sig_labels: dict[str, str] = {}

    def add_blob(self, label: str, data: bytes, vbase: int = 0, stride: int = 4):
        bid = len(self.blobs)
        self.blobs.append((label, data, vbase))
        idx = self.index
        crc = zlib.crc32
        for off in range(0, len(data) - WIN + 1, stride):
            w = data[off:off + WIN]
            if len(set(w)) < MIN_DISTINCT:
                continue
            idx.setdefault(crc(w), []).append((bid, off))

    def load_signatures(self, path: Path) -> int:
        n = 0
        if not path.exists():
            return 0
        for line in path.read_text().splitlines():
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            c, b, label = line.split(None, 2)
            self.sigs.setdefault(int(c, 16), set()).add(b)
            self.sig_labels[b] = label
            n += 1
        return n

    def lookup(self, w: bytes, width: int = 4):
        c = zlib.crc32(w)
        if c not in self.index and c not in self.sigs:
            return None
        if not window_ok(w, width):
            return None
        for bid, off in self.index.get(c, ()):
            label, data, vbase = self.blobs[bid]
            if data[off:off + WIN] == w:
                return (f"{label}+0x{off:X}" if not vbase
                        else f"{label} vaddr 0x{vbase + off:08X}")
        s = self.sigs.get(c)
        if s:
            b = blake8(w)
            if b in s:
                return f"signature {self.sig_labels[b]}"
        return None

    @property
    def empty(self) -> bool:
        return not self.index and not self.sigs


def load_checksums() -> dict[str, str]:
    out = {}
    if CHECKSUMS.exists():
        for line in CHECKSUMS.read_text().splitlines():
            f = line.split()
            if len(f) == 2:
                out[Path(f[1]).name] = f[0]
    return out


def sha256_file(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def load_disc(disc: Path, verbose=True):
    """[(label, payload bytes, vram base)] for every retail file present in
    disc/ whose sha256 matches config/checksum.sha."""
    sums = load_checksums()
    out = []
    for name, sumname, vbase, hdr in RETAIL_FILES:
        p = disc / name
        if not p.is_file():
            continue
        want = sums.get(sumname)
        if want and sha256_file(p) != want:
            if verbose:
                eprint(f"[guard] WARNING disc/{name} is not the retail USA file "
                       f"(sha256 mismatch vs config/checksum.sha); not used")
            continue
        out.append((name, p.read_bytes()[hdr:], vbase))
    p = disc / BIOS[0]
    if p.is_file() and sha256_file(p) == BIOS[1]:
        out.append((BIOS[0], p.read_bytes(), 0))
    return out


# -- candidate streams from a file ---------------------------------------------

def line_of(data: bytes, pos: int) -> int:
    return data.count(b"\n", 0, pos) + 1


def token_streams(data: bytes):
    """Yield (packed bytes, per-unit source positions, width) segments."""
    vals, poss = [], []
    for m in TOKEN_RE.finditer(data):
        t = m.group(0)
        v = int(t, 16) if t[:2] in (b"0x", b"0X") else int(t)
        vals.append(v)
        poss.append(m.start())
    if len(vals) < 8:
        return
    plans = [(vals, poss, 4), (vals, poss, 2), (vals, poss, 1),
             (vals[0::2], poss[0::2], 4), (vals[1::2], poss[1::2], 4)]
    for sv, sp, width in plans:
        need = WIN // width
        lim = 1 << (8 * width)
        bad = [i for i, v in enumerate(sv) if v >= lim]
        bad.append(len(sv))
        start = 0
        for e in bad:
            if e - start >= need:
                yield (array(_ARR[width], sv[start:e]).tobytes(), sp[start:e], width)
            start = e + 1


_ESC = {b"n": 10, b"t": 9, b"r": 13, b"0": 0, b"\\": 92, b'"': 34, b"'": 39,
        b"a": 7, b"b": 8, b"f": 12, b"v": 11, b"?": 63}


def _unescape(body: bytes) -> bytes:
    out = bytearray()
    i, n = 0, len(body)
    while i < n:
        c = body[i]
        if c != 0x5C or i + 1 >= n:
            out.append(c)
            i += 1
            continue
        e = body[i + 1:i + 2]
        if e == b"x":
            m = re.match(rb"[0-9A-Fa-f]{1,2}", body[i + 2:])
            if m:
                out.append(int(m.group(0), 16))
                i += 2 + len(m.group(0))
                continue
        if e.isdigit() and e not in (b"8", b"9"):
            m = re.match(rb"[0-7]{1,3}", body[i + 1:])
            out.append(int(m.group(0), 8) & 0xFF)
            i += 1 + len(m.group(0))
            continue
        out.append(_ESC.get(e, e[0]))
        i += 2
    return bytes(out)


def string_streams(data: bytes):
    """Yield (bytes, per-byte source positions, 1) for C string literals of
    >= 32 bytes; adjacent literals are joined like the C compiler does."""
    lits = list(STRING_RE.finditer(data))
    i = 0
    while i < len(lits):
        buf = bytearray()
        pos: list[int] = []
        j = i
        while True:
            b = _unescape(lits[j].group(1))
            buf += b
            pos += [lits[j].start()] * len(b)
            if j + 1 < len(lits) and JOIN_RE.fullmatch(data[lits[j].end() - 1:lits[j + 1].start() + 1]):
                j += 1
                continue
            break
        buf.append(0)
        pos.append(lits[j].end())
        if len(buf) >= WIN:
            yield bytes(buf), pos, 1
        i = j + 1


def _collect_runs(buf: bytes, pos: list[int], width: int, ref: Reference):
    n = len(pos)
    need = WIN // width
    out = []
    run = None  # [start_i, last_i, where, is_data]
    for i in range(0, n - need + 1):
        w = buf[i * width:i * width + WIN]
        where = ref.lookup(w, width)
        if not where:
            continue
        data_cls = not (width == 4 and address_only(w))
        if run is not None and i <= run[1] + need:
            run[1] = i
            run[3] = run[3] or data_cls
            continue
        if run is not None:
            out.append(run)
        run = [i, i, where, data_cls]
    if run is not None:
        out.append(run)
    return [(pos[r[0]], pos[r[1] + need - 1], (r[1] - r[0]) * width + WIN, r[2],
             "data" if r[3] else "address-table") for r in out]


def is_host_executable(data: bytes) -> bool:
    return data[:4] == b"\x7fELF" or data[:2] == b"MZ" or data[:4] in (
        b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe")


def scan_bytes(path: str, data: bytes, ref: Reference, allow: set[str]):
    """Return [(from, to, nbytes, match, class)]."""
    if path in allow or Path(path).suffix.lower() in SKIP_SUFFIXES:
        return []
    try:
        data.decode("utf-8")
        is_text = True
    except UnicodeDecodeError:
        is_text = False
    raw = []
    if is_text:
        for buf, pos, width in token_streams(data):
            raw.extend(_collect_runs(buf, pos, width, ref))
        for buf, pos, width in string_streams(data):
            raw.extend(_collect_runs(buf, pos, width, ref))
    else:
        if len(data) > MAX_BINARY or is_host_executable(data):
            return []
        raw.extend(_collect_runs(data, list(range(len(data))), 1, ref))
    raw.sort()
    merged = []
    for a, b, nb, where, cls in raw:
        if merged and a <= merged[-1][1]:
            m = merged[-1]
            m[1] = max(m[1], b)
            m[2] = max(m[2], nb)
            if cls == "data":
                m[4] = "data"
            continue
        merged.append([a, b, nb, where, cls])
    if is_text:
        return [(line_of(data, a), line_of(data, b), nb, w, c) for a, b, nb, w, c in merged]
    return [(f"byte 0x{a:X}", f"byte 0x{b:X}", nb, w, c) for a, b, nb, w, c in merged]


# -- file sets -------------------------------------------------------------------

def git(*args) -> bytes:
    return subprocess.run(["git", "-C", str(ROOT), *args], check=True,
                          stdout=subprocess.PIPE).stdout


def iter_files(args):
    if args.files:
        for f in args.files:
            p = Path(f)
            try:
                rel = str(p.resolve().relative_to(ROOT))
            except ValueError:
                rel = f
            yield rel, p.read_bytes()
        return
    if args.staged:
        names = git("diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z").split(b"\0")
        for n in names:
            if n:
                yield n.decode(), git("show", ":" + n.decode())
        return
    for n in git("ls-files", "-z").split(b"\0"):
        if not n:
            continue
        p = ROOT / n.decode()
        if p.is_file() and not p.is_symlink():
            yield n.decode(), p.read_bytes()


def load_allow() -> tuple[set[str], dict[str, int]]:
    """Allowlist rows: `path` (skip the whole file) or `path<TAB>max bytes`
    (a budget for the retail bytes matched in that path; the same bytes can
    match several retail files, so the budget is per path).  Must only SHRINK."""
    files: set[str] = set()
    budgets: dict[str, int] = {}
    if not ALLOW_FILE.exists():
        return files, budgets
    for line in ALLOW_FILE.read_text().splitlines():
        line = line.split("#", 1)[0].rstrip()
        if not line.strip():
            continue
        f = [x.strip() for x in line.split("\t")]
        if len(f) == 1:
            files.add(f[0])
        else:
            budgets[f[0]] = int(f[-1], 0)
    return files, budgets


# -- signature generation --------------------------------------------------------

def load_known_ranges():
    """[(retail file, file offset, size, label)] -- addresses only."""
    rows = []
    if KNOWN_FILE.exists():
        for line in KNOWN_FILE.read_text().splitlines():
            if not line.strip() or line.startswith("#"):
                continue
            f = line.split("\t")
            rows.append((f[0], int(f[1], 16), int(f[2], 16), f[3] if len(f) > 3 else ""))
    return rows


def write_signatures(blobs) -> int:
    names = {b[0] for b in blobs}
    missing = [n for n, *_ in RETAIL_FILES if n not in names]
    if missing:
        eprint(f"[guard] --write-signatures needs every retail file; missing/mismatched: {missing}")
        return 2
    by = {label: (data, vbase) for label, data, vbase in blobs}
    rows: dict[str, tuple[int, str]] = {}

    def add(w: bytes, label: str):
        if window_ok(w):
            b = blake8(w)
            if b not in rows:
                rows[b] = (zlib.crc32(w), label)

    for name, *_ in RETAIL_FILES:
        data, vbase = by[name]
        for off in range(0, len(data) - WIN + 1, SIG_SAMPLE_STRIDE):
            add(data[off:off + WIN], f"{name}@0x{vbase + off:08X}")
    for name, off, size, label in load_known_ranges():
        data, vbase = by[name]
        start = off - off % SIG_KNOWN_STRIDE
        for o in range(start, min(off + size, len(data)) - WIN + 1, SIG_KNOWN_STRIDE):
            add(data[o:o + WIN], f"{name}@0x{vbase + o:08X}")
    lines = ["# retail_data_guard signatures: crc32 blake2b-8 label.  One-way hashes",
             "# of 32-byte retail windows (never bytes); windows are >= 32 bytes apart",
             "# so they cannot be chained.  Regenerate with",
             "# tools/analysis/retail_data_guard.py --write-signatures (needs the disc).",
             f"# window={WIN} min_distinct={MIN_DISTINCT} sample_stride={SIG_SAMPLE_STRIDE} "
             f"known_stride={SIG_KNOWN_STRIDE}"]
    for b, (c, label) in sorted(rows.items(), key=lambda kv: kv[1][1]):
        lines.append(f"{c:08x} {b} {label}")
    SIG_FILE.write_text("\n".join(lines) + "\n")
    print(f"[guard] wrote {len(rows)} signatures to {SIG_FILE.relative_to(ROOT)}")
    return 0


def write_known_ranges(report, blobs) -> int:
    """Turn disc-matched runs into (file, offset, size) rows."""
    by = {label: (data, vbase) for label, data, vbase in blobs}
    spans: dict[str, list[list[int]]] = {}
    for r in report:
        if r["class"] != "data":
            continue
        m = re.match(r"(\S+) vaddr 0x([0-9A-F]+)$|(\S+)\+0x([0-9A-F]+)$", r["match"])
        if not m:
            continue
        name = m.group(1) or m.group(3)
        vb = by[name][1]
        off = (int(m.group(2), 16) - vb) if m.group(1) else int(m.group(4), 16)
        spans.setdefault(name, []).append([off, off + r["bytes"], r["file"]])
    lines = ["# Retail ranges that were once committed (retail file, file offset of the",
             "# payload -- after the 0x800 EXE header for SLUS_006.64 --, size, label).",
             "# Addresses and sizes only.  Used by retail_data_guard.py --write-signatures.",
             "# file\toffset\tsize\tlabel"]
    n = 0
    for name in sorted(spans):
        s = sorted(spans[name])
        merged = []
        for a, b, f in s:
            if merged and a <= merged[-1][1] + 64:
                merged[-1][1] = max(merged[-1][1], b)
                continue
            merged.append([a, b, f])
        for a, b, f in merged:
            lines.append(f"{name}\t0x{a:X}\t0x{b - a:X}\t{Path(f).stem}")
            n += 1
    KNOWN_FILE.write_text("\n".join(lines) + "\n")
    print(f"[guard] wrote {n} known ranges to {KNOWN_FILE.relative_to(ROOT)}")
    return 0


def write_allowlist(report, allow_files) -> int:
    budget: dict[str, int] = {}
    for r in report:
        if r["class"] == "data":
            budget[r["file"]] = budget.get(r["file"], 0) + r["bytes"]
    skipped = sorted(f for f in budget if f.startswith("pc_port/"))
    if skipped:
        eprint(f"[guard] not allowlisting pc_port files (they must be fixed): {skipped}")
    budget = {k: v for k, v in budget.items() if not k.startswith("pc_port/")}
    lines = ["# retail_data_guard allowlist: known committed retail runs pending removal.",
             "# Rows: path<TAB>max matched retail bytes (a budget that must only",
             "# SHRINK), or a bare path (file skipped).  Current rows are matching-build",
             "# sources (src/**, tools/**): inline-asm `.word` bodies and data tables of",
             "# not-yet-decompiled code.  pc_port/** may never be listed here."]
    lines += sorted(allow_files)
    lines += [f"{f}\t{v}" for f, v in sorted(budget.items())]
    ALLOW_FILE.write_text("\n".join(lines) + "\n")
    print(f"[guard] wrote {len(budget)} budget rows to {ALLOW_FILE.relative_to(ROOT)}")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("files", nargs="*")
    ap.add_argument("--staged", action="store_true")
    ap.add_argument("--no-disc", action="store_true", help="signatures only (CI)")
    ap.add_argument("--disc", default=str(DISC), help="directory with the retail files")
    ap.add_argument("--write-signatures", action="store_true")
    ap.add_argument("--write-known-ranges", action="store_true")
    ap.add_argument("--write-allowlist", action="store_true",
                    help="record every current hit (review before committing!)")
    ap.add_argument("--json", help="write hits as JSON here")
    ap.add_argument("--strict", action="store_true",
                    help="also fail on pointer/jump-table (address-only) runs")
    args = ap.parse_args(argv)

    blobs = [] if args.no_disc else load_disc(Path(args.disc))
    if args.write_signatures:
        return write_signatures(blobs)

    ref = Reference()
    nsig = ref.load_signatures(SIG_FILE)
    mode = [f"{nsig} signatures"]
    for label, data, vbase in blobs:
        ref.add_blob(label, data, vbase)
    if blobs:
        mode.append(f"{len(blobs)} disc file(s)")
    if ref.empty:
        eprint(f"[guard] no reference data (no disc files and no {SIG_FILE.relative_to(ROOT)})")
        return 2
    if args.write_known_ranges and not blobs:
        eprint("[guard] --write-known-ranges needs the disc files")
        return 2
    eprint(f"[guard] reference: {', '.join(mode)}")

    allow_files, allow_runs = load_allow()
    nfiles = 0
    report = []
    for path, data in iter_files(args):
        nfiles += 1
        for a, b, nb, where, cls in scan_bytes(path, data, ref, allow_files):
            report.append({"file": path, "from": a, "to": b, "bytes": nb,
                           "match": where, "class": cls})
    if args.write_known_ranges:
        return write_known_ranges(report, blobs)
    if args.write_allowlist:
        return write_allowlist(report, allow_files)
    used: dict[str, int] = {}
    for r in report:
        if r["class"] != "data":
            continue
        k = r["file"]
        if k in allow_runs:
            used[k] = used.get(k, 0) + r["bytes"]
            r["class"] = "allowlisted"
    over = sorted(k for k, v in used.items() if v > allow_runs[k])
    for k in over:
        for r in report:
            if r["class"] == "allowlisted" and r["file"] == k:
                r["class"] = "data"
        eprint(f"[guard] {k}: {used[k]} retail bytes, over the allowlisted {allow_runs[k]}")
    fails = [r for r in report if r["class"] == "data" or (args.strict and r["class"] != "allowlisted")]
    infos = [r for r in report if r["class"] == "address-table" and r not in fails]
    nallowed = sum(1 for r in report if r["class"] == "allowlisted")
    # Stale/shrinkable budgets are only judged on a full-tree scan with the
    # disc (signature-only mode sees a subset of what the disc sees).
    stale = []
    if not args.files and not args.staged and blobs:
        stale = sorted(k for k in allow_runs if used.get(k, 0) < allow_runs[k])
    for f in stale:
        print(f"STALE-ALLOW {f}: budget {allow_runs[f]} but {used.get(f, 0)} "
              f"bytes found -- lower or delete the row in {ALLOW_FILE.relative_to(ROOT)}")
    for r in fails[:300]:
        loc = f"{r['file']}:{r['from']}" + (f"-{r['to']}" if r["to"] != r["from"] else "")
        print(f"RETAIL-DATA {loc}: {r['bytes']} bytes match {r['match']}")
    if len(fails) > 300:
        print(f"... {len(fails) - 300} more (see --json)")
    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=1))
    if nallowed:
        eprint(f"[guard] note: {nallowed} allowlisted run(s) "
               f"({ALLOW_FILE.relative_to(ROOT)}; must only shrink)")
    if infos:
        eprint(f"[guard] note: {len(infos)} pointer/jump-table run(s) "
               f"(address facts, same as the symbol maps) -- fail only under --strict")
    if fails:
        files = sorted({r["file"] for r in fails})
        eprint(f"[guard] FAIL: {len(fails)} retail byte run(s) in {len(files)} file(s) "
               f"(scanned {nfiles}): {', '.join(files[:20])}{' ...' if len(files) > 20 else ''}")
        eprint("[guard] Load these bytes from the user's disc at runtime "
               "(pc_port/src/retail_data.h) instead of committing them.")
        return 1
    if stale:
        eprint(f"[guard] FAIL: {len(stale)} stale allowlist row(s)")
        return 1
    eprint(f"[guard] OK: {nfiles} file(s), no retail byte runs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
