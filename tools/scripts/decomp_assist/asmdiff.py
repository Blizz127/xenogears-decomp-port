#!/usr/bin/env python3
"""asmdiff.py OV NAME [OBJ] [--all]

Aligned side-by-side diff of NAME as compiled in OBJ against the retail
disassembly in asm/OV/**/NAME.s (the splat output).  OBJ defaults to the
tu_test.py object ($XENO_ASSIST_CACHE/<XENO_CCT_PREFIX or _tut>.o).
Relocated fields are masked the way compare() masks them (%hi/%lo/%gp_rel
immediates, j/jal targets), so a `!` line is a real code difference and a
`~` line differs only in a relocation.  Only differing regions are printed
with three lines of context unless --all is given.  Needs the build
container (mips-linux-gnu-objdump)."""
import difflib
import glob
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import CACHE, REPO

os.chdir(REPO)
args = [a for a in sys.argv[1:] if not a.startswith('--')]
ov, name = args[0], args[1]
obj = args[2] if len(args) > 2 else os.path.join(CACHE, os.environ.get('XENO_CCT_PREFIX', '_tut') + '.o')
show_all = '--all' in sys.argv

# retail: (word, mask, text)
retail = []
asm = glob.glob(f'asm/{ov}/**/{name}.s', recursive=True)
if not asm:
    sys.exit(f'no asm for {name} under asm/{ov}')
in_func = False  # only the function's own .text words: a .rodata jump table in the same file is not code
for line in open(asm[0]):
    if re.match(r'\s*glabel ', line) or line.startswith('.section'):
        in_func = bool(re.match(r'\s*glabel ' + re.escape(name) + r'\b', line))
        continue
    m = re.match(r'\s*/\* ([0-9A-F]+) ([0-9A-F]{8}) ([0-9A-F]{8}) \*/\s+(\S+)\s*(.*)', line)
    if not m or not in_func:
        continue
    word = int.from_bytes(bytes.fromhex(m[3]), 'little')
    mask = 0xFFFFFFFF
    if '%hi(' in m[5] or '%lo(' in m[5] or '%gp_rel' in m[5]:
        mask = 0xFFFF0000
    elif m[4] in ('jal', 'j'):
        mask = 0xFC000000
    retail.append((word, mask, (m[4] + ' ' + m[5]).strip()))

# compiled: objdump the function's range
nm = subprocess.run(['mips-linux-gnu-nm', '-S', obj], capture_output=True, text=True).stdout
sym = [l.split() for l in nm.splitlines() if len(l.split()) == 4 and l.split()[3] == name]
if not sym:
    sys.exit(f'{name} not in {obj}')
start, size = int(sym[0][0], 16), int(sym[0][1], 16)
dump = subprocess.run(['mips-linux-gnu-objdump', '-d', '-j', '.text', '-r', f'--start-address={start}',
                       f'--stop-address={start + size}', obj], capture_output=True, text=True).stdout
built = []  # (word, mask, text)
reloc = {}
for line in dump.splitlines():
    r = re.match(r'\s*([0-9a-f]+):\s+(R_MIPS_\S+)\s+(\S+)', line)
    if r:
        reloc[int(r[1], 16)] = (r[2], r[3])
        continue
    m = re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s+(\S+)\s*(.*)', line)
    if m:
        built.append((int(m[1], 16), int(m[2], 16), (m[3] + ' ' + m[4]).strip()))
tmp = []
for o, w, t in built:
    mask = 0xFFFFFFFF
    if o in reloc:
        kind, sym = reloc[o]
        mask = 0xFC000000 if kind == 'R_MIPS_26' else 0xFFFF0000
        t += f'  <{sym}>'
    tmp.append((w, mask, t))
built = tmp

# align on masked words, so spelling differences between splat and objdump
# (`$a1` vs `a1`, `li` vs `ori`) never count as differences
a = [f'{w & k:08x}' for w, k, _ in retail]
b = [f'{w & k:08x}' for w, k, _ in built]
rows = []  # (flag, left, right)
for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes():
    if tag == 'equal':
        for i, j in zip(range(i1, i2), range(j1, j2)):
            rows.append((' ', retail[i][2], built[j][2]))
    else:
        n = max(i2 - i1, j2 - j1)
        for x in range(n):
            lt = retail[i1 + x][2] if i1 + x < i2 else ''
            rt = built[j1 + x][2] if j1 + x < j2 else ''
            flag = '!' if lt and rt else ('<' if lt else '>')
            # same opcode/registers but a different relocation target: mark `~`
            if lt and rt and (retail[i1 + x][0] & 0xFC000000) == (built[j1 + x][0] & 0xFC000000) and \
                    retail[i1 + x][1] != 0xFFFFFFFF:
                flag = '~'
            rows.append((flag, lt, rt))

bad = sum(1 for f, _, _ in rows if f in '!<>')
print(f'# {name}: retail {len(retail)} insns, built {len(built)} insns, {bad} differing rows '
      f'({sum(1 for f, _, _ in rows if f == "~")} relocation-only)')
if show_all or bad == 0:
    sel = range(len(rows))
else:
    keep = set()
    for i, (f, _, _) in enumerate(rows):
        if f in '!<>':
            keep.update(range(max(0, i - 3), min(len(rows), i + 4)))
    sel = sorted(keep)
last = -1
for i in sel:
    if i != last + 1:
        print('   ...')
    f, lt, rt = rows[i]
    print(f'{i:4d} {f} {lt:44s} | {rt}')
    last = i
