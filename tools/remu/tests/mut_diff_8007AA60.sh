#!/usr/bin/env bash
# Mutant for diff_8007AA60: flip the multiply into an add.
# Usage: mut_diff_8007AA60.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "(u32)pRow[p[1]] * (u32)p[2]"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "(u32)pRow[p[1]] + (u32)p[2]"))
PY
