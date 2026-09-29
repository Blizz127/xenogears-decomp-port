#!/usr/bin/env bash
# Mutant for diff_8007AEF0: flip the saturating mul into an add.
# Usage: mut_diff_8007AEF0.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "prod = v1term * v2term;"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "prod = v1term + v2term;"))
PY
