#!/usr/bin/env bash
# Mutant for diff_8007B98C: swap copy direction in func_8007B98C only.
# Usage: mut_diff_8007B98C.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8007B98C(")
end = src.index("}", start)
body = src[start:end]
old = "t[p[2]] = t[p[1]];"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "t[p[1]] = t[p[2]];") + src[end:]
open(sys.argv[2], "w").write(src)
PY
