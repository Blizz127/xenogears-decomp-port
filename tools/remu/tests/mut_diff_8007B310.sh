#!/usr/bin/env bash
# Mutant for diff_8007B310: flip the tested OR into an AND.
# Usage: mut_diff_8007B310.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "row[p[3]] = row[p[1]] | row[p[2]];"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "row[p[3]] = row[p[1]] & row[p[2]];"))
PY
