#!/usr/bin/env bash
# Mutant for diff_8007D610: flip the presence check in func_8007D610 only.
# Usage: mut_diff_8007D610.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8007D610(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "if (D_800D2DCC[i] != 0"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "if (D_800D2DCC[i] == 0") + src[end:]
open(sys.argv[2], "w").write(src)
PY
