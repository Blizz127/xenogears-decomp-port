#!/usr/bin/env bash
# Mutant for diff_8007A7BC: halve the dst row stride (<<3 -> <<2).
# Usage: mut_diff_8007A7BC.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u8 func_8007A7BC(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "(((u32)a3 & 0xFFu) << 3)"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "(((u32)a3 & 0xFFu) << 2)") + src[end:]
open(sys.argv[2], "w").write(src)
PY
