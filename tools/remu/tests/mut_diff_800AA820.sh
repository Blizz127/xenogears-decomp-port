#!/usr/bin/env bash
# Mutant for diff_800AA820: route arg0==2 at the default entry (3490).
# Usage: mut_diff_800AA820.sh <tu-src> <mutant-out>
set -uo pipefail
python3 - "$1" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
start = src.index("void* func_800AA820(")
end = src.index("\n}\n", start)
body = src[start:end]
old = """    if (arg0 == 2) {
        return func_800A3578;
    }"""
assert body.count(old) == 1, body.count(old)
src = src[:start] + body.replace(old, """    if (arg0 == 2) {
        return func_800A3490;
    }""") + src[end:]
open(sys.argv[2], "w").write(src)
PY
