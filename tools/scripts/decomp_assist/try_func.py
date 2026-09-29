#!/usr/bin/env python3
"""try_func.py OV NAME [--keep]

Flip one function from `#ifndef XENO_PC_PORT / INCLUDE_ASM(...) / #else /
<C> / #endif` to plain C in its TU, rebuild only that object, and compare the
function with the retail words (difflib alignment when the sizes differ).
Reverts the flip on a mismatch unless --keep.  If the function is already C
it is just rebuilt and compared.  The object is deleted first because ninja
does not track every #included source."""
import difflib
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import REPO, compare, func_words, load_table, run_build

os.chdir(REPO)
OV, name = sys.argv[1], sys.argv[2]
table = load_table(OV)
seg = next(r for r, fl in table.items() if any(f['name'] == name for f in fl))
ins = next(f for f in table[seg] if f['name'] == name)['ins']
src = f'src/{OV}/{seg}.c'
s = open(src).read()
backup = None
guard = f'#ifndef XENO_PC_PORT\nINCLUDE_ASM("asm/{OV}/nonmatchings/{seg}", {name});\n#else\n'
if guard in s:
    backup = s
    st = s.index(guard)
    s = s[:st] + s[st + len(guard):]
    pos, depth = st, 0
    while True:  # matching #endif, skipping nested conditionals
        end = s.index('\n', pos)
        line = s[pos:end].strip()
        if re.match(r'#\s*if', line):
            depth += 1
        elif re.match(r'#\s*endif', line):
            if depth == 0:
                break
            depth -= 1
        pos = end + 1
    s = s[:pos] + s[end + 1:]
    open(src, 'w').write(s)
elif f', {name});' in s and 'INCLUDE_ASM' in s.split(f', {name});')[0].rsplit('\n', 1)[-1]:
    sys.exit(f'{name} is INCLUDE_ASM with no C body in {src} (draft one with m2c_try.py)')
obj = f'build/src/{OV}/{seg}.c.o'
for ext in ('.o', '.s', '.i'):
    if os.path.exists(f'build/src/{OV}/{seg}.c{ext}'):
        os.remove(f'build/src/{OV}/{seg}.c{ext}')
r = run_build(f'nice -n 10 ninja {obj} 2>&1 | grep -iE "error" | head -20')
print(r.stdout[-3000:])
got = func_words(obj, name) if os.path.exists(obj) else None


def revert():
    if backup is not None and '--keep' not in sys.argv:
        open(src, 'w').write(backup)
        print('reverted flip')


if got is None:
    revert()
    sys.exit('symbol not in object')
size, words = got
print(f'{name}: built {size} retail {len(ins) * 4}')
text = {}
for line in open(f'asm/{OV}/{seg}.s'):
    m = re.match(r'\s*/\* ([0-9A-F]+) [0-9A-F]{8} [0-9A-F]{8} \*/\s+(.*)', line)
    if m:
        text[int(m[1], 16)] = m[2].strip()
bad = compare(ins, words)
if bad > 0:
    for i, ((o, w, k), b) in enumerate(zip(ins, words)):
        if (b & k) != (w & k):
            print(f'  +{4 * i:#x} R {text.get(o, "")} | B {b:08x}')
elif bad < 0:
    def mk(x):
        op = x >> 26
        if op in (2, 3):
            return x & 0xFC000000
        if op in (0x0F, 0x09, 0x20, 0x21, 0x23, 0x24, 0x25, 0x28, 0x29, 0x2B):
            return x & 0xFFFF0000
        return x
    a = [mk(w) for o, w, k in ins]
    b = [mk(x) for x in words]
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes():
        if tag != 'equal':
            print(f'  {tag} R[{i1}:{i2}] B[{j1}:{j2}]:', '; '.join(text.get(ins[i][0], '') for i in range(i1, min(i2, i1 + 6))))
print('RESULT', 'MATCH' if bad == 0 else f'NOMATCH bad={bad}')
if bad != 0:
    revert()
