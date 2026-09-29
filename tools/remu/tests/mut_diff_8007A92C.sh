#!/usr/bin/env bash
# Mutant for diff_8007A92C: shift the high byte by 7 instead of 8.
# Usage: mut_diff_8007A92C.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "(u16)((p[3] << 8) | p[2])"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "(u16)((p[3] << 7) | p[2])"))
PY
