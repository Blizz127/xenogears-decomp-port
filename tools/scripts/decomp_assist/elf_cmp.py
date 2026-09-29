#!/usr/bin/env python3
"""elf_cmp.py OV VRAM

Masked per-function comparison of the linked build/out/OV.elf/.bin against
the retail words from asm/OV.  Prints functions whose size or code differs
(relocation targets are masked; use cmp/rom-check for the final verdict)."""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import REPO, load_table

os.chdir(REPO)
ov, vram = sys.argv[1], int(sys.argv[2], 16)
built = {}
for line in subprocess.run(['mips-linux-gnu-nm', '-S', f'build/out/{ov}.elf'],
                           capture_output=True, text=True).stdout.splitlines():
    p = line.split()
    if len(p) == 4 and p[2] in 'tT':
        built[p[3]] = (int(p[0], 16), int(p[1], 16))
data = open(f'build/out/{ov}.bin', 'rb').read()
for seg, funcs in load_table(ov).items():
    for f in funcs:
        n, ins = f['name'], f['ins']
        if n not in built:
            print(n, 'missing')
            continue
        addr, size = built[n]
        if size != len(ins) * 4:
            print(n, f'size {size} vs {len(ins) * 4}')
            continue
        o = addr - vram
        if any((int.from_bytes(data[o + 4 * i:o + 4 * i + 4], 'little') & k) != (w & k)
               for i, (x, w, k) in enumerate(ins)):
            print(n, 'diff')
