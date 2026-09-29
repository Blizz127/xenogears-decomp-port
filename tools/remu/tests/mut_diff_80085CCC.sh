#!/usr/bin/env bash
# Mutant for diff_80085CCC: break the 0x2DC decrement (0xFF -> 0xFE).
# Usage: mut_diff_80085CCC.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085CCC(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "D_800D2CAA = (u8)(b + 0xFFu);"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "D_800D2CAA = (u8)(b + 0xFEu);") + src[end:]
open(sys.argv[2], "w").write(src)
PY
