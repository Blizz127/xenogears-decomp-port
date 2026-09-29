#!/usr/bin/env bash
# Mutant for diff_8007AE38: flip the saturating add into a sub.
# Usage: mut_diff_8007AE38.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "(s32)pRow[p[1]] + (s32)pRow[p[2]]"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "(s32)pRow[p[1]] - (s32)pRow[p[2]]"))
PY
