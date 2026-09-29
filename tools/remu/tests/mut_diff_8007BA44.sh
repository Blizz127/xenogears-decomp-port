#!/usr/bin/env bash
# Mutant for diff_8007BA44: swap the p[1]/p[2] roles in func_8007BA44 only.
# Usage: mut_diff_8007BA44.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8007BA44(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "*(u32 *)(f2 + t_base) = val;"
assert body.count(old) == 1, body.count(old)
new = "*(u32 *)(f1 + t_base) = val;"
src = src[:start] + body.replace(old, new) + src[end:]
open(sys.argv[2], "w").write(src)
PY
