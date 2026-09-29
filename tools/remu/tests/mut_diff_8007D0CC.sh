#!/usr/bin/env bash
# Mutant for diff_8007D0CC: scan only 8 entries (0x30 -> 0x08).
# Usage: mut_diff_8007D0CC.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_8007D0CC(")
end = src.index("\n}\n", start)
body = src[start:end]
old = "for (i = 0; i < 0x30; i++) {"
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, "for (i = 0; i < 0x08; i++) {") + src[end:]
open(sys.argv[2], "w").write(src)
PY
