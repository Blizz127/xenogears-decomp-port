#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N54_OUT:-pc_port/build_native/w34n54-model-bounds-cert}"
TEST="pc_port/tests/w34n54_model_bounds_cull_prod_test.c"
PROD="pc_port/src/model_ordering.c"
CALLER="src/slus_006.64/system/temp2.c"
BASE=(-std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
      -DINCLUDE_ASM_USE_MACRO_INC=0 -fno-pie
      -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include
      -Ipc_port/extern/PsyCross/include/psx -Iinclude -Ipc_port/src)
WARN=(-Wall -Wextra -Werror -Wconversion -Wsign-conversion
      -Wshadow -Wundef)

mkdir -p "$OUT"

rg -q 'extern s32 func_8003101C\(const u8\* bounds, s32 mode\);' "$CALLER"
rg -q 'func_8003101C\(a0, D_80050104\)' "$CALLER"

compile_and_run() {
    local name="$1"
    local rc=0
    shift
    rm -f "$OUT/$name" "$OUT/$name.stdout" "$OUT/$name.stderr"
    "$CC" "${BASE[@]}" "${WARN[@]}" "$@" "$TEST" "$PROD" \
        -no-pie -o "$OUT/$name"
    if "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"; then
        rc=0
    else
        rc=$?
    fi
    return "$rc"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2 -g
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^W34N54 0x8003101C model-bounds culler certificate PASS$' \
        "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

set +e
compile_and_run hidden_sample -O0 -g -DXENO_TEST_MODEL_ORDERING_HIDE_SAMPLE
rc=$?
set -e
if [[ $rc -eq 0 ]] || ! rg -q '^ASSERTION mode1.visible_early_out$' "$OUT/hidden_sample.stderr"; then
    echo "Hiding the visible sample did not fail the expected visibility assertion" >&2
    exit 1
fi
echo "W34N54 model ordering PASS; hidden projection sample detected"
