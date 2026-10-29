#!/usr/bin/env python3
"""Retail-data guard for the LINKED port executable.

retail_data_guard.py scans committed sources (numeric literals, strings,
binary files) and skips host executables.  Tables written as structured C
initialisers (structs, mixed widths, expressions) escape its token streams
but compile to the exact retail bytes, so a release must also be checked
after linking.  This scans every byte of the given ELF against the same
reference (the user's retail files in disc/ plus the BIOS image when
present, else the one-way signatures) and maps each 32-byte-or-longer run
to the symbol that owns it.

Exit status: 0 no retail runs, 1 runs found, 2 usage/config error.

  retail_binary_guard.py [--disc DIR] [--json OUT] BINARY
"""

from __future__ import annotations

import argparse
import bisect
import importlib.util
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
CHUNK = 1 << 20
OVERLAP = 256


def load_guard():
    spec = importlib.util.spec_from_file_location("retail_data_guard", HERE / "retail_data_guard.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def elf_sections(binary: str):
    """[(file_off, file_end, vaddr, name)] for sections with file contents."""
    out = []
    for line in subprocess.check_output(["readelf", "-SW", binary], text=True).splitlines():
        parts = line.replace("[ ", "[").split()
        if len(parts) < 7 or not parts[0].startswith("[") or parts[0] == "[Nr]":
            continue
        try:
            name, addr, off, size = parts[1], int(parts[3], 16), int(parts[4], 16), int(parts[5], 16)
        except ValueError:
            continue
        if size and parts[2] != "NOBITS":
            out.append((off, off + size, addr, name))
    return out


def elf_symbols(binary: str):
    syms = []
    for line in subprocess.check_output(["nm", "-S", "--defined-only", binary], text=True).splitlines():
        parts = line.split()
        if len(parts) == 4:
            syms.append((int(parts[0], 16), int(parts[1], 16), parts[3]))
    syms.sort()
    return syms


def owner(offset, sections, syms, starts):
    for off, end, vaddr, name in sections:
        if off <= offset < end:
            va = vaddr + (offset - off)
            i = bisect.bisect_right(starts, va) - 1
            if i >= 0 and syms[i][0] <= va < syms[i][0] + max(syms[i][1], 1):
                return name, syms[i][2]
            return name, "?"
    return "-", "?"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("binary")
    ap.add_argument("--disc", default=None, help="directory with the retail files (default: repo disc/)")
    ap.add_argument("--json", help="write the runs as JSON here")
    args = ap.parse_args(argv)

    g = load_guard()
    ref = g.Reference()
    nsig = ref.load_signatures(g.SIG_FILE) if g.SIG_FILE.exists() else 0
    blobs = g.load_disc(Path(args.disc) if args.disc else g.DISC)
    for label, data, vbase in blobs:
        ref.add_blob(label, data, vbase)
    if ref.empty:
        print("[binary-guard] no reference data (no disc files, no signature file)", file=sys.stderr)
        return 2
    print(f"[binary-guard] reference: {nsig} signatures, {len(blobs)} disc file(s)", file=sys.stderr)

    data = Path(args.binary).read_bytes()
    runs = {}
    for base in range(0, len(data), CHUNK):
        seg = data[base:base + CHUNK + OVERLAP]
        for a, b, nb, where, cls in g._collect_runs(seg, list(range(base, base + len(seg))), 1, ref):
            key = (a, where)
            if key not in runs or runs[key][2] < nb:
                runs[key] = (a, b, nb, where, cls)
    sections, syms = elf_sections(args.binary), elf_symbols(args.binary)
    starts = [s[0] for s in syms]
    report = []
    for a, b, nb, where, cls in sorted(runs.values()):
        if report and a <= report[-1]["to"]:
            report[-1]["to"] = max(report[-1]["to"], b)
            report[-1]["bytes"] = max(report[-1]["bytes"], nb)
            continue
        sec, sym = owner(a, sections, syms, starts)
        report.append({"offset": a, "to": b, "bytes": nb, "match": where, "section": sec, "symbol": sym})
    for r in report:
        print(f"RETAIL-DATA {args.binary}+0x{r['offset']:X}: {r['bytes']} bytes match {r['match']} "
              f"({r['section']} {r['symbol']})")
    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=1))
    total = sum(r["bytes"] for r in report)
    if report:
        print(f"[binary-guard] FAIL: {len(report)} retail run(s), {total} bytes, in {args.binary}", file=sys.stderr)
        return 1
    print(f"[binary-guard] OK: no retail byte runs in {args.binary} ({len(data)} bytes scanned)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
