#!/usr/bin/env bash
# Mutant for diff_80085C48: narrow the 98C6C arg mask (0xFFFF -> 0xFF).
# Usage: mut_diff_80085C48.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085C48(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "func_80098C6C(a2 & 0xFFFF);"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "func_80098C6C(a2 & 0xFF);") + src[end:]
open(sys.argv[2], "w").write(src)
PY
