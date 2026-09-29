#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
OUT="${W34N39_OUT:-pc_port/build_native/w34n39-89748-cert}"
TEST="pc_port/tests/w34n39_89748_prod_test.c"
PROD="pc_port/src/world_map_helper_89748.c"
BASE=(-std=gnu17 -DXENO_PC_PORT -DINCLUDE_ASM_USE_MACRO_INC=0 -fno-pie -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Iinclude -Ipc_port/src)
WARN=(-Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -Wundef)

mkdir -p "$OUT"

compile_and_run() {
    local name="$1"
    local rc=0
    shift
    rm -f "$OUT/$name" "$OUT/$name.stdout" "$OUT/$name.stderr"
    "$CC" "${BASE[@]}" "${WARN[@]}" "$@" "$TEST" "$PROD" -no-pie -o "$OUT/$name"
    if "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"; then
        rc=0
    else
        rc=$?
    fi
    return "$rc"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2 -g
compile_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^W34N39 0x80089748 full-body certificate PASS$' "$OUT/$regime.stdout"
    test ! -s "$OUT/$regime.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; focused warnings clean"

mutants=(
    'M1:WM_89748_MUTANT_SOURCE_COUNT_256:walks-512-sources-at-54-byte-stride'
    'M2:WM_89748_MUTANT_SOURCE_STRIDE_4C:walks-512-sources-at-54-byte-stride'
    'M3:WM_89748_MUTANT_FLAGS_AT_4B:walks-512-sources-at-54-byte-stride'
    'M4:WM_89748_MUTANT_HIGH_TIMER_ZERO:walks-512-sources-at-54-byte-stride'
    'M5:WM_89748_MUTANT_FREE_TEST_LOW_HALF:free-test-uses-packed-high-halfword'
    'M6:WM_89748_MUTANT_WRONG_FIRST_VECTOR:retail-matrix-and-vector-addresses'
    'M7:WM_89748_MUTANT_RANDOM_X_FIRST:retail-random-order-and-normal-inputs'
    'M8:WM_89748_MUTANT_WRONG_FIRST_AMPLITUDE:spawned-first-and-second-vectors'
    'M9:WM_89748_MUTANT_SKIP_SECOND_APPLY:retail-matrix-and-vector-addresses'
    'M10:WM_89748_MUTANT_UNSIGNED_COPIES:spawned-heading-and-signed-fields'
    'M11:WM_89748_MUTANT_NO_SOURCE_COUNT_INCREMENT:walks-512-sources-at-54-byte-stride'
    'M12:WM_89748_MUTANT_NO_TICK:particle-tick-runs-once-after-source-walk'
)

for entry in "${mutants[@]}"; do
    label="${entry%%:*}"
    rest="${entry#*:}"
    define="${rest%%:*}"
    assertion="${rest#*:}"
    set +e
    compile_and_run "$label" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! rg -q "^ASSERTION $assertion$" "$OUT/$label.stderr"; then
        echo "$label FAILED mutant gate assertion=$assertion rc=$rc" >&2
        exit 1
    fi
    echo "$label DETECTED; ASSERTION $assertion"
done

echo "W34N39 0x80089748 FULL CERTIFICATE PASS; M1-M12 DETECTED"
