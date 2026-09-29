#!/usr/bin/env python3
"""Classify SLUS_006.64 address ranges and symbols as Sony Psy-Q SDK code.

Adapted from the Parasite Eve decomp's tools/analysis/psyq_classify.py.  In
Xenogears the splat config already files the linked SDK under psyq/<lib>
(config/slus_006.64.yaml), so the classification is read from there:

  * every subsegment named psyq/... is SDK code/rodata of that library, from
    its start to the next subsegment's start;
  * the unnamed `asm` island 0x3F080 ("Likely part of libcard") sits between
    psyq/libcard and .sdata and is filed as libcard;
  * data tables the SDK owns inside the shared .sdata are listed by hand in
    config/psyq_data_ranges.tsv (address ranges only, no bytes).

Symbols: every function in config/symbol_addrs.slus_006.64.txt whose address
lies in a Psy-Q code range, plus every `func_XXXXXXXX` in those ranges (by
address, see in_psyq_code()).

Output (--write): config/psyq_classification.tsv with columns
    kind  start  end  library  name
(kind = range | func).  Names/addresses only -- no SDK bytes, no SDK source.
"""
from __future__ import annotations

import argparse
import bisect
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
YAML = ROOT / "config/slus_006.64.yaml"
SYMS = ROOT / "config/symbol_addrs.slus_006.64.txt"
OUT = ROOT / "config/psyq_classification.tsv"
DATA_RANGES = ROOT / "config/psyq_data_ranges.tsv"
ROM_TO_VRAM = 0x80010000 - 0x800
TEXT_END_ROM = 0x3F290  # first .sdata subsegment
UNNAMED_PSYQ_ASM = {0x3F080: "psyq/libcard"}

SUBSEG_RE = re.compile(r"^\s*-\s*\[(0x[0-9A-Fa-f]+)\s*,\s*([.\w]+)(?:\s*,\s*([^\]\s#]+))?\s*\]")


def load_ranges() -> list[tuple[int, int, str, str]]:
    """[(vram start, vram end, library, section)] for Psy-Q code/rodata."""
    subs: list[tuple[int, str, str]] = []
    in_main = False
    for line in YAML.read_text().splitlines():
        if re.match(r"\s*- name: main\s*$", line):
            in_main = True
            continue
        if not in_main:
            continue
        if re.match(r"\s*- \[0x49AC0", line):
            break
        s = line.split("#", 1)[0] if not line.lstrip().startswith("#") else ""
        m = SUBSEG_RE.match(s)
        if m:
            subs.append((int(m.group(1), 16), m.group(2), (m.group(3) or "").strip()))
    subs.sort()
    out = []
    for i, (rom, kind, name) in enumerate(subs):
        end = subs[i + 1][0] if i + 1 < len(subs) else TEXT_END_ROM
        if not name and rom in UNNAMED_PSYQ_ASM:
            name = UNNAMED_PSYQ_ASM[rom]
        if not name.startswith("psyq/"):
            continue
        lib = name.split("/")[1]
        out.append((rom + ROM_TO_VRAM, end + ROM_TO_VRAM, lib,
                    "rodata" if "rodata" in kind else "text"))
    return out


def load_data_ranges() -> list[tuple[str, int, int, str]]:
    """[(name, vram start, vram end, library)] from config/psyq_data_ranges.tsv."""
    rows = []
    if DATA_RANGES.exists():
        for line in DATA_RANGES.read_text().splitlines():
            if not line.strip() or line.startswith(("#", "name\t")):
                continue
            f = line.split("\t")
            rows.append((f[0], int(f[1], 16), int(f[2], 16), f[3] if len(f) > 3 else ""))
    return rows


class Classification:
    def __init__(self):
        self.ranges = [r for r in load_ranges() if r[3] == "text"]
        self.starts = [r[0] for r in self.ranges]
        self.names: dict[str, tuple[int, str]] = {}
        for line in SYMS.read_text().splitlines():
            m = re.match(r"\s*([A-Za-z_]\w*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;(.*)", line)
            if not m or "type:data" in m.group(3) or "size:" in m.group(3):
                continue
            a = int(m.group(2), 16)
            lib = self.library(a)
            if lib:
                self.names[m.group(1)] = (a, lib)

    def library(self, addr: int) -> str | None:
        i = bisect.bisect_right(self.starts, addr) - 1
        if i >= 0 and self.ranges[i][0] <= addr < self.ranges[i][1]:
            return self.ranges[i][2]
        return None


def write(path: Path, quiet: bool = False) -> int:
    c = Classification()
    lines = ["# Psy-Q classification of SLUS_006.64 (tools/analysis/psyq_classify.py).",
             "# Addresses and names only.  kind\tstart\tend\tlibrary\tname"]
    for lo, hi, lib, sec in load_ranges():
        lines.append(f"range\t0x{lo:08X}\t0x{hi:08X}\t{lib}\t{sec}")
    for name, lo, hi, lib in load_data_ranges():
        lines.append(f"data\t0x{lo:08X}\t0x{hi:08X}\t{lib}\t{name}")
    for name, (a, lib) in sorted(c.names.items(), key=lambda kv: kv[1][0]):
        lines.append(f"func\t0x{a:08X}\t-\t{lib}\t{name}")
    path.write_text("\n".join(lines) + "\n")
    if not quiet:
        print(f"psyq_classify: wrote {len(lines) - 2} rows to {path.relative_to(ROOT)}")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--write", action="store_true", help=f"write {OUT.relative_to(ROOT)}")
    ap.add_argument("--check", action="store_true", help="fail if the TSV is stale")
    a = ap.parse_args(argv)
    if a.write:
        return write(OUT)
    if a.check:
        tmp = OUT.with_suffix(".tsv.tmp")
        write(tmp, quiet=True)
        same = tmp.read_text() == (OUT.read_text() if OUT.exists() else "")
        tmp.unlink()
        if not same:
            print("psyq_classify: config/psyq_classification.tsv is stale; rerun --write")
            return 1
        print("psyq_classify: config/psyq_classification.tsv is current")
        return 0
    c = Classification()
    for lo, hi, lib, sec in load_ranges():
        print(f"0x{lo:08X}-0x{hi:08X} {lib:8s} {sec}")
    print(f"{len(c.names)} named Psy-Q functions")
    return 0


if __name__ == "__main__":
    sys.exit(main())
