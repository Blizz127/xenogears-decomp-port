#!/usr/bin/env bash
# Mutant for diff_8007BAE8: shift the slot base (+3 -> +4).
# Usage: mut_diff_8007BAE8.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8007BAE8(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "(a1 & 0xFF) + 3"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "(a1 & 0xFF) + 4") + src[end:]
open(sys.argv[2], "w").write(src)
PY
