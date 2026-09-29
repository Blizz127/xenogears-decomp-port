#!/usr/bin/env bash
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "row[(*pp)[1]] = result;"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "row[(*pp)[2]] = result;"))
PY
