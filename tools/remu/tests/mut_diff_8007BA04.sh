#!/usr/bin/env bash
# Mutant for diff_8007BA04: write to the wrong halfword row slot.
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8007BA04(")
end = src.index("\n}", start)
body = src[start:end]
old = "idx2 = p[2];"
assert body.count(old) == 1
src = src[:start] + body.replace(old, "idx2 = p[1];") + src[end:]
open(sys.argv[2], "w").write(src)
PY
