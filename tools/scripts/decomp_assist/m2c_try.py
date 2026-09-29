#!/usr/bin/env python3
"""m2c_try.py OV SEG [names...]

Draft each INCLUDE_ASM function of segment SEG (asm/OV/nonmatchings/SEG/*.s)
with m2c (-t mips-gcc-c --valid-syntax), compile it standalone with the
repo's cc1 + maspsx on the host, and compare against the retail words.
Writes CACHE/m2c/OV/SEG/<name>.{c,i,s,o} and results.json; prints one line
per function (MATCH / diff n/m / size a vs b / *-fail).

Draft fix-ups (each learned from a real mismatch):
  * m2c drops unused leading parameters; they are added back, because the
    retail code reads its operands from the original argument registers.
  * AUTO_VOID=1 (default): an unknown callee whose result is never used is
    declared void.  m2c's int guess keeps v0 live across the call and changes
    register allocation (found by decomp-permuter on bcf1 801E6144).
  * FORCE_VOID="fn ..." / FORCE_TYPES="sym=type ..." override guesses.
  * ARRAYIFY="sym ..."|all declares scalar externs as arrays and accesses
    element 0 (retail keeps such an address in a register).
  * KR_PROTOS=1 turns m2c's guessed extern prototypes into unprototyped ones.
  * VARIANTS=1 (default) also tries ARRAYIFY=all, KR_PROTOS=1 and both,
    keeping the first variant that matches (its files stay in the cache).
A standalone MATCH is not proof: insert with m2c_insert.py and rebuild."""
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import CACHE, M2C, REPO, compare, func_words, load_table, toolchain

os.chdir(REPO)
OV, SEG = sys.argv[1], sys.argv[2]
names = sys.argv[3:]
D = os.path.join(CACHE, 'm2c', OV, SEG)
os.makedirs(D, exist_ok=True)
funcs = {f['name']: f for f in load_table(OV)[SEG]}
names = names or list(funcs)
# compiler, -G and maspsx flags of this TU's gears.toml preset
TC = toolchain(OV, SEG)
print(f"# {OV}/{SEG}: preset {TC['preset']}, {TC['cc1']}", flush=True)
M2C_DIR = os.path.dirname(M2C)


def fix_args(m):
    params = [x.strip() for x in m[2].split(',')]
    if m[2].strip() in ('void', ''):
        return m[0]
    idx = {}
    for x in params:
        mm = re.search(r'arg(\d+)$', x)
        if not mm:
            return m[0]
        idx[int(mm[1])] = x
    out = [idx.get(i, f's32 arg{i}') for i in range(max(idx) + 1)]
    return f'{m[1]}({", ".join(out)}) {{'


def try_one(n, env):
        asm = f'asm/{OV}/nonmatchings/{SEG}/{n}.s'
        if not os.path.exists(asm):
            return 'no-asm'
        c = subprocess.run(['python3', M2C, '-t', 'mips-gcc-c', '--valid-syntax', asm],
                           capture_output=True, text=True).stdout
        if 'Decompilation failure' in c or not c.strip():
            return 'm2c-fail'
        c = re.sub(r'^(\w[\w\s\*]*?\w+)\(([^)]*)\) \{', fix_args, c, flags=re.M)
        if env.get('AUTO_VOID', '1') == '1':
            tail = c.split('/* extern */')[-1]
            for fn in re.findall(r'^M2C_UNK\s+(\w+)\(', c, flags=re.M):
                uses = re.findall(r'(.{0,4})\b' + fn + r'\(', tail)
                if uses and all(u.strip() == '' for u in uses):
                    c = re.sub(r'^M2C_UNK\s+' + fn + r'\(', f'void {fn}(', c, flags=re.M)
        if env.get('KR_PROTOS'):
            c = re.sub(r'^(\w[\w\s\*]*?\b\w+)\([^)]*\);(\s*/\* extern \*/)', r'\1();\2', c, flags=re.M)
        for fn in env.get('FORCE_VOID', '').split():
            c = re.sub(r'^M2C_UNK\s+' + fn + r'\(', f'void {fn}(', c, flags=re.M)
        for kv in env.get('FORCE_TYPES', '').split():
            sym, t = kv.split('=')
            c = re.sub(r'^extern [^;]*\b' + sym + ';', f'extern {t} {sym};', c, flags=re.M)
        arr = env.get('ARRAYIFY', '').split()
        if arr:
            head, sep, body = c.partition('/* extern */')
            for m in re.finditer(r'^extern ([\w\s\*]+?)\s*\b(\w+);', c, flags=re.M):
                sym = m[2]
                if 'all' in arr or sym in arr:
                    c = c.replace(m[0], f'extern {m[1]} {sym}[];')
                    decl_end = c.index(f'{sym}[];') + len(sym) + 3
                    c = c[:decl_end] + re.sub(r'(&\s*)?\b' + sym + r'\b(?!\s*\[)', lambda mm: sym if mm[1] else sym + '[0]', c[decl_end:])
        base = os.path.join(D, n)
        open(base + '.c', 'w').write('#include "common.h"\n#include "m2c_macros.h"\n' + c)
        if subprocess.run(['tools/gcc-2.7.2-psx/cpp', '-Iinclude', '-I' + M2C_DIR, '-D_LANGUAGE_C', '-DSKIP_ASM',
                           '-P', '-undef', '-lang-c', '-nostdinc', base + '.c', base + '.i'],
                          capture_output=True).returncode:
            return 'cpp-fail'
        p = subprocess.run([TC['cc1']] + TC['cc1_flags'] + ['-o', base + '.s', base + '.i'],
                           capture_output=True, text=True)
        if p.returncode:
            return 'cc1-fail: ' + (p.stderr.strip().split('\n') or [''])[-1][-90:]
        p = subprocess.run(['python3', 'tools/maspsx/maspsx.py'] + TC['maspsx_flags'] + ['-o', base + '.o', base + '.s'],
                           capture_output=True, text=True, stdin=subprocess.DEVNULL)
        got = func_words(base + '.o', n) if os.path.exists(base + '.o') else None
        if got is None:
            return 'as-fail'
        bad = compare(funcs[n]['ins'], got[1])
        return ('MATCH' if bad == 0 else f'diff {bad}/{len(got[1])}') if bad >= 0 else \
            f'size {got[0]} vs {len(funcs[n]["ins"]) * 4}'


VARIANTS = [dict(os.environ)]
if os.environ.get('VARIANTS', '1') == '1':
    for extra in ({'ARRAYIFY': 'all'}, {'KR_PROTOS': '1'}, {'ARRAYIFY': 'all', 'KR_PROTOS': '1'}):
        e = dict(os.environ); e.update(extra); VARIANTS.append(e)
res = {}
for n in names:
    first = None
    for vi, env in enumerate(VARIANTS):
        r = try_one(n, env)
        first = first or r
        if r == 'MATCH':
            res[n] = 'MATCH' if vi == 0 else f'MATCH'
            break
        if r in ('no-asm', 'm2c-fail'):
            break
    else:
        # leave the base variant's files for inspection
        try_one(n, VARIANTS[0])
    res.setdefault(n, first)
for n, v in res.items():
    print(n, v)
json.dump(res, open(os.path.join(D, 'results.json'), 'w'), indent=1)
