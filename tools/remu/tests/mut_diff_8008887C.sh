#!/usr/bin/env bash
# Mutant for diff_8008887C: corrupt the pinned axis step in func_8008887C only.
# Usage: mut_diff_8008887C.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8008887C(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "D_800C3A90 = 0x100;"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "D_800C3A90 = 0x101;") + src[end:]
open(sys.argv[2], "w").write(src)
PY
