#!/usr/bin/env bash
# Mutant for diff_80085E78: shorten the fill loop (7 -> 6).
# Usage: mut_diff_80085E78.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085E78(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "for (i = 0; i < 7; i++) {"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "for (i = 0; i < 6; i++) {") + src[end:]
open(sys.argv[2], "w").write(src)
PY
