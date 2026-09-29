#!/usr/bin/env bash
# Mutant for diff_80085EB4: break the dispatch count (12 - a3 -> 11 - a3).
# Usage: mut_diff_80085EB4.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u32 func_80085EB4(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "u32 x = 12u - a3;"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "u32 x = 11u - a3;") + src[end:]
open(sys.argv[2], "w").write(src)
PY
