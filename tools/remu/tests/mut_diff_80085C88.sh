#!/usr/bin/env bash
# Mutant for diff_80085C88: move the trailing store (0xAD -> 0xAC).
# Usage: mut_diff_80085C88.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085C88(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "D_800D2D28[0xAD] = 0;"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "D_800D2D28[0xAC] = 0;") + src[end:]
open(sys.argv[2], "w").write(src)
PY
