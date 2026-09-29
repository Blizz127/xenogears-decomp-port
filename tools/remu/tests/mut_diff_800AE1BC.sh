#!/usr/bin/env bash
# Mutant for diff_800AE1BC: mirror +0x9E from +0x2 instead of +0x12.
# Usage: mut_diff_800AE1BC.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u32 func_800AE1BC(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "*(u16*)(a0 + 0x9E) = *(u16*)(a1 + 0x12);"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "*(u16*)(a0 + 0x9E) = *(u16*)(a1 + 0x2);") + src[end:]
open(sys.argv[2], "w").write(src)
PY
