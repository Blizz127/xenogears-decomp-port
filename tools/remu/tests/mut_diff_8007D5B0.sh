#!/usr/bin/env bash
# Mutant for diff_8007D5B0: flip the mask condition in func_8007D5B0 only.
# Usage: mut_diff_8007D5B0.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8007D5B0(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "(*(u16*)(D_800CCD64 + off) & 0xC000) == 0"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "(*(u16*)(D_800CCD64 + off) & 0xC000) != 0") + src[end:]
open(sys.argv[2], "w").write(src)
PY
