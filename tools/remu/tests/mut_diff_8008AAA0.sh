#!/usr/bin/env bash
# Mutant for diff_8008AAA0: shrink the digit loop in func_8008AAA0 only.
# Usage: mut_diff_8008AAA0.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8008AAA0(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "} while (i < 9);"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "} while (i < 8);") + src[end:]
open(sys.argv[2], "w").write(src)
PY
