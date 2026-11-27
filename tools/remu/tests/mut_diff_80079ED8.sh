#!/usr/bin/env bash
# Mutant for diff_80079ED8: halve the per-case row stride.
# Usage: mut_diff_80079ED8.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u8 func_80079ED8(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "a0 * 368"
assert body.count(old) == 24, body.count(old)
src = src[:start] + body.replace(old, "a0 * 184") + src[end:]
open(sys.argv[2], "w").write(src)
PY
