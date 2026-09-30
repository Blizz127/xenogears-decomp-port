#!/usr/bin/env bash
# Mutant for diff_80085618: invert the bit15 gate (!= 0 -> == 0).
# Usage: mut_diff_80085618.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085618(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "if ((h & 0x8000u) != 0) {"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "if ((h & 0x8000u) == 0) {") + src[end:]
open(sys.argv[2], "w").write(src)
PY
