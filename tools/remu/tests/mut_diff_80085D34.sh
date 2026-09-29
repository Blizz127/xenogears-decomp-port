#!/usr/bin/env bash
# Mutant for diff_80085D34: wrong middle-round a2 (0x32 -> 0x33).
# Usage: mut_diff_80085D34.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085D34(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "0x32u, 0xD0u);"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "0x33u, 0xD0u);") + src[end:]
open(sys.argv[2], "w").write(src)
PY
