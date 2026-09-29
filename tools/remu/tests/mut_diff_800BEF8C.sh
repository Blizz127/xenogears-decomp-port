#!/usr/bin/env bash
# Mutant for diff_800BEF8C: read the wrong unsigned half (0xA4 -> 0xA6)
# in func_800BEF8C only.
# Usage: mut_diff_800BEF8C.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u32 func_800BEF8C(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "((u32)*(u16 *)(arg0 + 0xA4) << 16)"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "((u32)*(u16 *)(arg0 + 0xA6) << 16)") + src[end:]
open(sys.argv[2], "w").write(src)
PY
