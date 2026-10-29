#!/usr/bin/env python3
"""Emit one deliberately-wrong copy of SOURCE per mutant (fn, old, new) listed in MUTFILE
(a python literal list); old must occur exactly once in the function body."""
import sys, re, ast
from pathlib import Path
src, out, mf = sys.argv[1], Path(sys.argv[2]), sys.argv[3]
s = Path(src).read_text()
muts = ast.literal_eval(Path(mf).read_text())
names = []
for fn, old, new in muts:
    a = b = -1
    for m in re.finditer(r'(?m)^[A-Za-z_][^\n;]*\b%s\([^;{]*\{\s*$' % re.escape(fn), s):
        e = s.find('\n}\n', m.start())
        if s.find(old, m.start(), e) != -1:
            a, b = m.start(), e; break
    assert a != -1, (fn, 'body with mutant text not found')
    body = s[a:b]
    assert body.count(old) == 1, (fn, old, body.count(old))
    (out / (fn + '.mut.c')).write_text(s[:a] + body.replace(old, new) + s[b:])
    names.append(fn)
(out / 'controls.txt').write_text('\n'.join(names) + '\n')
