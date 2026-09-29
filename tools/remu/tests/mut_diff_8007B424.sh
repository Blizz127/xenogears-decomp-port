#!/usr/bin/env bash
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
old = "row[p[1]] = value;"
assert src.count(old) == 1
open(sys.argv[2], "w").write(src.replace(old, "row[p[2]] = value;"))
PY
