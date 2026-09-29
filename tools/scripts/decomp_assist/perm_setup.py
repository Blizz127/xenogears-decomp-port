#!/usr/bin/env python3
"""perm_setup.py OV SEG NAME [BASE.c]

Create a decomp-permuter directory CACHE/perm/NAME: base.c (the given C file,
or m2c_try.py's preprocessed draft), target.o assembled from
asm/OV/nonmatchings/SEG/NAME.s, compile.sh (repo cc1 + maspsx on the host)
and settings.toml.  Then, sharing the box politely:
  nice -n 15 python3 permuter.py -j1 --best-only --stop-on-zero CACHE/perm/NAME
A BASE.c must already be preprocessed (e.g. the TU's .i with the other
function bodies removed)."""
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import CACHE, REPO, toolchain

ov, seg, name = sys.argv[1:4]
base = sys.argv[4] if len(sys.argv) > 4 else os.path.join(CACHE, 'm2c', ov, seg, name + '.i')
d = os.path.join(CACHE, 'perm', name)
os.makedirs(d, exist_ok=True)
shutil.copy(base, os.path.join(d, 'base.c'))
asm = open(os.path.join(REPO, f'asm/{ov}/nonmatchings/{seg}/{name}.s')).read()
open(os.path.join(d, 'target.s'), 'w').write('.include "macro.inc"\n.set noat\n.set noreorder\n.section .text\n' + asm)
subprocess.run(['mips-linux-gnu-as', '-EL', '-march=r3000', '-mtune=r3000', '-no-pad-sections',
                '-I' + os.path.join(REPO, 'include'), '-o', os.path.join(d, 'target.o'), os.path.join(d, 'target.s')],
               check=True, capture_output=True)
TC = toolchain(ov, seg)  # the TU's gears.toml preset (XENO_CC1/XENO_GP_FLAG override)
maspsx = ' '.join({'-Iinclude': '-I' + os.path.join(REPO, 'include'), 'build': os.path.join(REPO, 'build')}.get(f, f)
                  for f in TC['maspsx_flags'])
cc1 = TC['cc1'] if os.path.isabs(TC['cc1']) else os.path.join(REPO, TC['cc1'])
open(os.path.join(d, 'compile.sh'), 'w').write(f'''#!/bin/bash
# compile.sh in.c -o out.o  (maspsx reads stdin when it is not a TTY: keep </dev/null)
{cc1} {' '.join(TC['cc1_flags'])} -o "$3.s" "$1" && \\
python3 {REPO}/tools/maspsx/maspsx.py {maspsx} -o "$3" "$3.s" 2>/dev/null </dev/null
''')
os.chmod(os.path.join(d, 'compile.sh'), 0o755)
open(os.path.join(d, 'settings.toml'), 'w').write(f'func_name = "{name}"\ncompiler_type = "gcc"\n')
print(d)
