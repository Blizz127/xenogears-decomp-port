#!/usr/bin/env bash
# Mutant for diff_800B00D0: clear 8 words instead of 9 (i >= 0 -> i > 0).
# Usage: mut_diff_800B00D0.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "    for (i = 8; i >= 0; i--) {"
assert src.count(old) == 1, src.count(old)
open(sys.argv[2], "w").write(src.replace(old, "    for (i = 8; i > 0; i--) {"))
PY
