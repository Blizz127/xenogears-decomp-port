#!/usr/bin/env bash
# Mutant for diff_8007B8D4: store from the wrong party-table index.
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "row[p[1]] = D_8005A3A0[p[2]];"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "row[p[2]] = D_8005A3A0[p[2]];"))
PY
