#!/usr/bin/env bash
# Mutant for diff_80080AE4: drop the target exclusion (v1 != target -> 1).
# Usage: mut_diff_80080AE4.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("s32 func_80080AE4(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "if (*slot < lim && v != target) {"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "if (*slot < (s16)lim) {") + src[end:]
open(sys.argv[2], "w").write(src)
PY
