#!/usr/bin/env bash
# Mutant for diff_800BEEB4: flip the NULL check in func_800BEEB4 only.
# Usage: mut_diff_800BEEB4.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("u32 func_800BEEB4(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "if (entry != 0) {"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "if (entry == 0) {") + src[end:]
open(sys.argv[2], "w").write(src)
PY
