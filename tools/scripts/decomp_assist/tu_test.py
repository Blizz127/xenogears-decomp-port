#!/usr/bin/env python3
"""tu_test.py OV NAME BODY.c [--write]

Compile a candidate C definition of NAME *inside its own TU*, on the host,
without touching the tree: the TU source is read, NAME's INCLUDE_ASM site
(plain, or the whole `#ifndef XENO_PC_PORT / INCLUDE_ASM / #else <port body>
/ #endif` block) is replaced by BODY.c's text, and the result is
preprocessed (-I include and the TU's own directory), compiled with the TU's
gears.toml preset and compared with the retail words.  This catches the
in-context differences a standalone draft misses (the TU's own types for
globals and callees).  Prints MATCH / diff / size; the object is
$XENO_ASSIST_CACHE/<XENO_CCT_PREFIX or _tut>.o.  `make rom-check` is still
the authority.

--write: on a MATCH, write the edited TU back (BODY.c's `#include "common.h"`
line dropped), so the verified text is exactly what lands.  Only for plain
sites: one with a port body is left alone (m2c_insert.py --inner keeps it)."""
import glob
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import CACHE, REPO, compare, func_words, toolchain

ov, name, body_path = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3])
os.chdir(REPO)
body = open(body_path).read()
if '--write' in sys.argv:
    body = re.sub(r'^#include "common.h"\n', '', body, flags=re.M).strip('\n') + '\n'
site = re.compile(r'INCLUDE_ASM\("(?:\.\./)?asm/' + re.escape(ov) + r'/nonmatchings/([^"]+)", ' + re.escape(name) + r'\);')
tu = None
for f in glob.glob(f'src/{ov}/**/*.c', recursive=True):
    s = open(f).read()
    m = site.search(s)
    if m:
        tu, seg = f, m.group(1)
        break
if tu is None:
    sys.exit(f'no INCLUDE_ASM site for {name} under src/{ov}')
st, en = m.start(), m.end()
pre = s[:st].rsplit('\n', 2)
ported = False
if pre[-2].strip() == '#ifndef XENO_PC_PORT' and s[en:].startswith('\n#else\n'):
    st = len(s[:st]) - len(pre[-1]) - len(pre[-2]) - 1
    pos, depth = en + len('\n#else\n'), 0
    while True:
        end = s.index('\n', pos)
        line = s[pos:end].strip()
        if re.match(r'#\s*if', line):
            depth += 1
        elif re.match(r'#\s*endif', line):
            if depth == 0:
                break
            depth -= 1
        pos = end + 1
    en = end
    ported = True
    body = '#ifndef XENO_PC_PORT\n' + body + '\n#endif\n'
s = s[:st] + body + s[en:]
edited = s
# the TU in the matching build: drop other INCLUDE_ASM bodies (SKIP_ASM does that)
pre_ = os.environ.get('XENO_CCT_PREFIX', '_tut')
c_file, i_file, s_file, o_file = (os.path.join(CACHE, pre_ + e) for e in ('.c', '.i', '.s', '.o'))
open(c_file, 'w').write(s)
tc = toolchain(ov, seg if '/' not in tu[len(f'src/{ov}/'):] else tu[len(f'src/{ov}/'):-2])
p = subprocess.run(['tools/gcc-2.7.2-psx/cpp', '-Iinclude', '-I', os.path.dirname(tu), '-D_LANGUAGE_C', '-DSKIP_ASM',
                    '-P', '-undef', '-lang-c', '-nostdinc', c_file, i_file], capture_output=True, text=True)
if p.returncode:
    sys.exit(p.stderr[-2000:])
p = subprocess.run([tc['cc1']] + tc['cc1_flags'] + ['-o', s_file, i_file], capture_output=True, text=True)
if p.returncode:
    sys.exit('cc1-fail: ' + '\n'.join(p.stderr.strip().split('\n')[-5:]))
if os.path.exists(o_file):
    os.remove(o_file)
subprocess.run(['python3', 'tools/maspsx/maspsx.py'] + tc['maspsx_flags'] + ['-o', o_file, s_file],
               capture_output=True, stdin=subprocess.DEVNULL)
asm = glob.glob(f'asm/{ov}/**/{name}.s', recursive=True)[0]
ins = []
for line in open(asm):
    mm = re.match(r'\s*/\* ([0-9A-F]+) ([0-9A-F]{8}) ([0-9A-F]{8}) \*/\s+(\S+)\s*(.*)', line)
    if mm:
        word = int.from_bytes(bytes.fromhex(mm[3]), 'little')
        mask = 0xFFFFFFFF
        if '%hi(' in mm[5] or '%lo(' in mm[5] or '%gp_rel' in mm[5]:
            mask = 0xFFFF0000
        elif mm[4] in ('jal', 'j'):
            mask = 0xFC000000
        ins.append((int(mm[1], 16), word, mask))
got = func_words(o_file, name)
if not got:
    sys.exit('no symbol')
bad = compare(ins, got[1])
print(f"{tc['preset']}:", 'MATCH' if bad == 0 else (f'diff {bad}/{len(ins)}' if bad > 0 else f'size {got[0]} vs {len(ins) * 4}'))
if bad == 0 and '--write' in sys.argv and ported:
    sys.exit('not written: the site has a port body (use m2c_insert.py --inner)')
if bad == 0 and '--write' in sys.argv:
    open(tu, 'w').write(edited)
    print('wrote', tu)
