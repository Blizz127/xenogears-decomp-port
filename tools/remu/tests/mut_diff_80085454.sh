#!/usr/bin/env bash
# Mutant for diff_80085454: fold with (A - T) instead of |T - A|.
# Usage: mut_diff_80085454.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void func_80085454(")
end = src.index("\n}\n", start)
body = src[start:end]
old = """            } else if (w == 0 || w == 5) {
                s32 d = (s32)(s16)T - (s32)(s16)(*a2k);
                *a2k = (u16)(d < 0 ? -d : d);"""
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, """            } else if (w == 0 || w == 5) {
                s32 d = (s32)(s16)T - (s32)(s16)(*a2k);
                *a2k = (u16)d;""") + src[end:]
open(sys.argv[2], "w").write(src)
PY
