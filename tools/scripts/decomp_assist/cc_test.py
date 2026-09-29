#!/usr/bin/env python3
"""cc_test.py OV NAME FILE.c [cc1 ...]

Compile a hand-written C file standalone (repo cpp, cc1 and maspsx) and
compare function NAME with the retail words; prints MATCH / diff / size per
compiler.  By default it uses the gears.toml preset of NAME's segment (cc1,
cc1 flags and maspsx flags such as --expand-div or --dont-expand-li); naming
cc1 builds (e.g. gcc-2.7.2-psx gcc-2.6.3-psx) tries those with the same
preset flags.  XENO_MASPSX_EXTRA adds maspsx flags.  Scratch files are
$XENO_ASSIST_CACHE/<XENO_CCT_PREFIX or _cct>.{i,s,o}, so parallel runs need
distinct prefixes.  FILE.c should include "common.h" and declare what it
uses."""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import CACHE, REPO, compare, func_words, load_table, toolchain

ov, name, src = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3])
os.chdir(REPO)
seg, ins = next((s, f['ins']) for s, fl in load_table(ov).items() for f in fl if f['name'] == name)
tc = toolchain(ov, seg)
ccs = sys.argv[4:] or [os.path.basename(os.path.dirname(tc['cc1']))]
os.makedirs(CACHE, exist_ok=True)
pre = os.environ.get('XENO_CCT_PREFIX', '_cct')
i_file, s_file, o_file = (os.path.join(CACHE, pre + e) for e in ('.i', '.s', '.o'))
p = subprocess.run(['tools/gcc-2.7.2-psx/cpp', '-Iinclude', '-D_LANGUAGE_C', '-DSKIP_ASM', '-P', '-undef',
                    '-lang-c', '-nostdinc', src, i_file], capture_output=True, text=True)
if p.returncode:
    sys.exit(p.stderr)
maspsx = tc['maspsx_flags'] + os.environ.get('XENO_MASPSX_EXTRA', '').split()
for cc in ccs:
    tag = f"{cc} [{tc['preset']}]"
    p = subprocess.run([f'tools/{cc}/cc1'] + tc['cc1_flags'] + ['-o', s_file, i_file], capture_output=True, text=True)
    if p.returncode:
        print(tag, 'cc1-fail:', p.stderr.strip().split('\n')[-1])
        continue
    if os.path.exists(o_file):
        os.remove(o_file)
    subprocess.run(['python3', 'tools/maspsx/maspsx.py'] + maspsx + ['-o', o_file, s_file],
                   capture_output=True, stdin=subprocess.DEVNULL)
    got = func_words(o_file, name)
    if not got:
        print(tag, 'no symbol')
        continue
    bad = compare(ins, got[1])
    print(tag, 'MATCH' if bad == 0 else (f'diff {bad}/{len(ins)}' if bad > 0 else f'size {got[0]} vs {len(ins) * 4}'))
