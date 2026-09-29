#!/usr/bin/env bash
# Mutant for diff_80077844: rotate the last store (dst[8] = i -> dst[8] = h).
# Usage: mut_diff_80077844.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "    dst[8] = i;"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "    dst[8] = h;"))
PY
