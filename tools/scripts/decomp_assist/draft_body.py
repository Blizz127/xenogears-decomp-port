#!/usr/bin/env python3
"""draft_body.py OV SEG NAME [OUT.c]

Emit the m2c_try.py draft of NAME ($XENO_ASSIST_CACHE/m2c/OV/SEG/NAME.c) as a
TU-ready body for tu_test.py: the two draft #includes are dropped, M2C_FIELD
becomes a raw-offset access and M2C_UNK becomes s32 (the same cleaning
m2c_insert.py applies).  The extern/prototype lines m2c guessed are kept so the
body compiles when inserted; delete the ones the TU already declares if cc1
reports a conflicting type.  Writes OUT.c (default: NAME.body.c next to the
draft) and prints its path."""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from assist_common import CACHE

ov, seg, name = sys.argv[1], sys.argv[2], sys.argv[3]
src = os.path.join(CACHE, 'm2c', ov, seg, name + '.c')
out = sys.argv[4] if len(sys.argv) > 4 else os.path.join(CACHE, 'm2c', ov, seg, name + '.body.c')
c = open(src).read()
for inc in ('#include "common.h"\n', '#include "m2c_macros.h"\n'):
    c = c.replace(inc, '', 1)


def clean(c):
    while 'M2C_FIELD(' in c:
        i = c.rindex('M2C_FIELD(')
        j = i + len('M2C_FIELD(')
        d, args, cur = 1, [], ''
        while d:
            ch = c[j]
            if ch == '(':
                d += 1
            elif ch == ')':
                d -= 1
                if d == 0:
                    break
            if ch == ',' and d == 1:
                args.append(cur)
                cur = ''
            else:
                cur += ch
            j += 1
        args.append(cur)
        e, t, o = [a.strip() for a in args]
        c = c[:i] + f'*({t})((s8*)({e}) + {o})' + c[j + 1:]
    return c.replace('M2C_UNK', 's32')


open(out, 'w').write(clean(c))
print(out)
