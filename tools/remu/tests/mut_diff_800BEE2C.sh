#!/usr/bin/env bash
# Mutant for diff_800BEE2C: narrow the a0 mask in func_800BEE2C only.
# Usage: mut_diff_800BEE2C.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_800BEE2C(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "func_800AA320(arg0 & 0xFFFF, arg1 & 0xFFFF, arg2)"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "func_800AA320(arg0 & 0xFF, arg1 & 0xFFFF, arg2)") + src[end:]
open(sys.argv[2], "w").write(src)
PY
