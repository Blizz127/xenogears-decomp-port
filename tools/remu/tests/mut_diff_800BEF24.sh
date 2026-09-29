#!/usr/bin/env bash
# Mutant for diff_800BEF24: swap the callee arg order in func_800BEF24 only.
# Usage: mut_diff_800BEF24.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u32 func_800BEF24(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "func_80023124((s32)a0, (s32)a1)"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "func_80023124((s32)a1, (s32)a0)") + src[end:]
open(sys.argv[2], "w").write(src)
PY
