#!/usr/bin/env bash
# Mutant for diff_800AF400: flip the tested bit condition.
# Usage: mut_diff_800AF400.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "((D_800C3E30 >> bit) & 1) == 0"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "((D_800C3E30 >> bit) & 1) != 0"))
PY
