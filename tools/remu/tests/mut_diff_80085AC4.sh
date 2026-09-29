#!/usr/bin/env bash
# Mutant for diff_80085AC4: corrupt the deepest countdown store (0 -> 0x404).
# Usage: mut_diff_80085AC4.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085AC4(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "*(u16*)(D_800CCD74 + off - 8) = 0;"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "*(u16*)(D_800CCD74 + off - 8) = 0x404;") + src[end:]
open(sys.argv[2], "w").write(src)
PY
