#!/usr/bin/env bash
# Mutant for diff_8007A6C8: shrink the 368-row stride (<<4 -> <<3).
# Usage: mut_diff_8007A6C8.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u32 func_8007A6C8(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "    off <<= 4;\n"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "    off <<= 3;\n") + src[end:]
open(sys.argv[2], "w").write(src)
PY
