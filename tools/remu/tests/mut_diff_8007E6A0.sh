#!/usr/bin/env bash
# Mutant for diff_8007E6A0: turn the add into a sub in func_8007E6A0 only.
# Usage: mut_diff_8007E6A0.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8007E6A0(")
end = src.index("}", start)
body = src[start:end]
old = "t[p[3]] = t[p[1]] + t[p[2]];"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "t[p[3]] = t[p[1]] - t[p[2]];") + src[end:]
open(sys.argv[2], "w").write(src)
PY
