#!/usr/bin/env bash
# Mutant for diff_800AA898: store 0 instead of 1 at +0x8E.
# Usage: mut_diff_800AA898.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u32 func_800AA898(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "*(u16*)(p + 0x8E) = 1;"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "*(u16*)(p + 0x8E) = 0;") + src[end:]
open(sys.argv[2], "w").write(src)
PY
