#!/usr/bin/env bash
# Mutant for diff_8007A968: shift the assembled word by 5 instead of 4.
# Usage: mut_diff_8007A968.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "(u32)(((p[3] << 8) | p[2]) << 4)"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "(u32)(((p[3] << 8) | p[2]) << 5)"))
PY
