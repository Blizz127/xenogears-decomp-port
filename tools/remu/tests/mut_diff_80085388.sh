#!/usr/bin/env bash
# Mutant for diff_80085388: stride 70 instead of 72 in the first store.
# Usage: mut_diff_80085388.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085388(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "*(u16*)(base + (s32)v * 72 + (i << 1)) = 0;"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "*(u16*)(base + (s32)v * 70 + (i << 1)) = 0;") + src[end:]
open(sys.argv[2], "w").write(src)
PY
