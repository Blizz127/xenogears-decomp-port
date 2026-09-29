#!/usr/bin/env bash
# Mutant for diff_80088990: break the negative rounding in func_80088990 only.
# Usage: mut_diff_80088990.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80088990(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "v += 0xFF;"
assert body.count(old) == 2, body.count(old)
src = src[:start] + body.replace(old, "v += 0xFE;") + src[end:]
open(sys.argv[2], "w").write(src)
PY
