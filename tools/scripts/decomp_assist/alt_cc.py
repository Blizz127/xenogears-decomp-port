#!/usr/bin/env python3
"""alt_cc.py OV

First pass for stubborn register-allocation / `mfhi` near-misses: recompile
the cached m2c_try.py drafts (CACHE/m2c/OV/**/<name>.i) of every function
that did not match with each alternative PSX cc1 in tools/ (gcc-2.6.3-psx,
gcc-2.6.0-psx, gcc-2.7.2-cdk-psx), and, when the retail asm has `break`
(aspsx divide checks), each compiler with maspsx --expand-div too; report the
functions that match under one of them.  Host-only, no m2c rerun.  A hit means the TU may belong under
that compiler's gears.toml preset: the whole TU must stay byte-exact under it
(rebuild and rom-check).  Writes CACHE/altcc_OV.json."""
import glob
import json
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import CACHE, CC1_FLAGS, MASPSX_FLAGS, REPO, compare, func_words, load_table

COMPILERS = ('gcc-2.6.3-psx', 'gcc-2.6.0-psx', 'gcc-2.7.2-cdk-psx')
os.chdir(REPO)
ov = sys.argv[1]
funcs = {f['name']: f for fl in load_table(ov).values() for f in fl}
hits = {}
for rj in glob.glob(os.path.join(CACHE, 'm2c', ov, '**', 'results.json'), recursive=True):
    d = os.path.dirname(rj)
    seg = os.path.relpath(d, os.path.join(CACHE, 'm2c', ov))
    for n, v in json.load(open(rj)).items():
        if v == 'MATCH' or n not in funcs or not os.path.exists(f'{d}/{n}.i'):
            continue
        if not os.path.exists(f'asm/{ov}/nonmatchings/{seg}/{n}.s'):
            continue  # already C
        has_break = 'break' in open(f'asm/{ov}/nonmatchings/{seg}/{n}.s').read()
        for cc, div in [(c, d) for c in ('gcc-2.7.2-psx',) + COMPILERS for d in ((False, True) if has_break else (False,))]:
            if cc == 'gcc-2.7.2-psx' and not div:
                continue  # the m2c_try baseline
            s_file, o_file = os.path.join(CACHE, '_alt.s'), os.path.join(CACHE, '_alt.o')
            if subprocess.run([f'tools/{cc}/cc1'] + CC1_FLAGS + ['-o', s_file, f'{d}/{n}.i'],
                              capture_output=True).returncode:
                continue
            if os.path.exists(o_file):
                os.remove(o_file)
            subprocess.run(['python3', 'tools/maspsx/maspsx.py'] + (['--expand-div'] if div else []) + MASPSX_FLAGS + ['-o', o_file, s_file],
                           capture_output=True, stdin=subprocess.DEVNULL)
            got = func_words(o_file, n) if os.path.exists(o_file) else None
            if got and compare(funcs[n]['ins'], got[1]) == 0:
                hits.setdefault(n, {'seg': seg, 'compilers': []})['compilers'].append(cc + (' +expand-div' if div else ''))
for n, h in sorted(hits.items()):
    print(n, h['seg'], ' '.join(h['compilers']))
json.dump(hits, open(os.path.join(CACHE, f'altcc_{ov}.json'), 'w'), indent=1)
