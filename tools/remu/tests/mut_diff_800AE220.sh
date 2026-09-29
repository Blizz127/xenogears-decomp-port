#!/usr/bin/env bash
# Mutant for diff_800AE220: route a1==1 through the +0xB4 chain.
# Usage: mut_diff_800AE220.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u32 func_800AE220(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "p = (u8*)(uintptr_t)(*(u32*)(a0 + 0xB0));"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "p = (u8*)(uintptr_t)(*(u32*)(a0 + 0xB4));") + src[end:]
open(sys.argv[2], "w").write(src)
PY
