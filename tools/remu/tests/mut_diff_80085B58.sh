#!/usr/bin/env bash
# Mutant for diff_80085B58: shift the C4000 mark (s1 + 8 -> s1 + 9).
# Usage: mut_diff_80085B58.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085B58(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "D_800C4000[s2] = (u8)(s1 + 8u);"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "D_800C4000[s2] = (u8)(s1 + 9u);") + src[end:]
open(sys.argv[2], "w").write(src)
PY
