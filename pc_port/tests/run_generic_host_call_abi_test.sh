#!/usr/bin/env bash
# Characterizes the generic host-call ABI mismatch (see
# generic_host_call_abi_test.c).  Expected today:
#   * gcc O0/O2/UBSan: the mismatched call works and is NOT detected;
#   * clang -fsanitize=function: the mismatched call is rejected, while
#     the typed control passes.
# The test fails if any of that changes, including when the production
# typedef no longer has the twelve-uintptr_t shape, so the record is updated
# together with a fix.  Not a pass of the boundary itself: it reproduces a
# known limit in isolation.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/generic_host_call_abi_test.XXXXXXXX)
echo "GENERIC HOST CALL artifacts: $OUT"
python3 - "$OUT/generic_host_function.h" <<'PY'
import re, sys
from pathlib import Path
source = Path('pc_port/src/battle_mips_runtime.c').read_text()
found = re.findall(r'typedef uintptr_t \(\*GenericHostFunction\)\(([^;]*)\);', source)
assert len(found) == 1, 'GenericHostFunction typedef not found exactly once'
params = [p.strip() for p in found[0].split(',')]
assert params == ['uintptr_t'] * 12, 'GenericHostFunction shape changed: ' + found[0]
assert source.count('function = (GenericHostFunction)resolved->host;') == 1
Path(sys.argv[1]).write_text('#include <stdint.h>\ntypedef uintptr_t (*GenericHostFunction)(' + found[0] + ');\n')
PY
src=pc_port/tests/generic_host_call_abi_test.c
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    [ "$opt" = UBSan ] && flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    "${GCC:-gcc}" -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Wno-cast-function-type \
        -I"$OUT" "$src" -o "$OUT/gcc.$opt"
    for mode in typed mismatch; do
        "$OUT/gcc.$opt" "$mode" > "$OUT/gcc.$opt.$mode.log" 2>&1 || {
            cat "$OUT/gcc.$opt.$mode.log" >&2
            echo "GENERIC HOST CALL FAIL gcc $opt $mode" >&2; exit 1; }
    done
done
"${CLANG:-clang}" -std=gnu17 -O1 -fsanitize=function -fno-sanitize-recover=all \
    -Wall -Wextra -Werror -Wno-cast-function-type-mismatch -Wno-unknown-warning-option \
    -I"$OUT" "$src" -o "$OUT/clang.function"
"$OUT/clang.function" typed > "$OUT/clang.typed.log" 2>&1 || {
    cat "$OUT/clang.typed.log" >&2; echo "GENERIC HOST CALL FAIL clang typed control" >&2; exit 1; }
if "$OUT/clang.function" mismatch > "$OUT/clang.mismatch.log" 2>&1; then
    echo "GENERIC HOST CALL FAIL clang -fsanitize=function no longer rejects the mismatch; update this record" >&2
    exit 1
fi
grep -q 'through pointer to incorrect function type' "$OUT/clang.mismatch.log"
echo "GENERIC HOST CALL KNOWN LIMIT REPRODUCED: gcc O0/O2/UBSan run the mismatched call undetected;"
echo "  clang -fsanitize=function rejects it ($(grep -o 'call to function [A-Za-z]*' "$OUT/clang.mismatch.log" | head -1)), typed control passes"
