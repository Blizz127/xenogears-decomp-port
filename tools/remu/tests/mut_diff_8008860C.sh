#!/usr/bin/env bash
# Mutant for diff_8008860C: corrupt the 0x5D83 copy in func_8008860C only.
# Usage: mut_diff_8008860C.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8008860C(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "base[0x5D83] = D_800CCB34;"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "base[0x5D83] = (u8)(D_800CCB34 + 1u);") + src[end:]
open(sys.argv[2], "w").write(src)
PY
