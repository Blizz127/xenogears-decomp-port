#!/usr/bin/env bash
# Mutant for diff_8007D344: shift the C3EB7 base (0x54 -> 0x50).
# Usage: mut_diff_8007D344.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8007D344(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "u32 s3 = 0x54;"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "u32 s3 = 0x50;") + src[end:]
open(sys.argv[2], "w").write(src)
PY
