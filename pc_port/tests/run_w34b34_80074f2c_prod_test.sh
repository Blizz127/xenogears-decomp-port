#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${W34B34_OUT:-pc_port/build_native/w34b34-cert}"
mkdir -p "$OUT"
FLAGS=(
    -std=gnu17 -g -Wall -Wextra -Wconversion -Wsign-conversion -Werror
    -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
)
TEST="pc_port/tests/w34b34_80074f2c_prod_test.c"
PUMP="pc_port/src/world_map_upload_pump_74f2c.c"

build_run() {
    local label="$1" opt="$2" san="${3:-}" defs="${4:-}"
    gcc -c "$TEST" "${FLAGS[@]}" "$opt" $san $defs -o "$OUT/$label.test.o"
    gcc -c "$PUMP" "${FLAGS[@]}" "$opt" $san $defs -o "$OUT/$label.pump.o"
    gcc -no-pie -Wl,--gc-sections $san "$OUT/$label.test.o" \
        "$OUT/$label.pump.o" -o "$OUT/$label"
    set +e
    "$OUT/$label" > "$OUT/$label.stdout" 2> "$OUT/$label.stderr"
    local rc=$?
    set -e
    echo "$rc" > "$OUT/$label.rc"
}

build_run O0 -O0
build_run O2 -O2
build_run UBSan_O2 -O2 "-fsanitize=undefined -fno-sanitize-recover=all"
for label in O0 O2 UBSan_O2; do
    test "$(<"$OUT/$label.rc")" = 0
    rg -q '^=== Results: 7/7 PASS ===$' "$OUT/$label.stdout"
    ! rg -qi 'runtime error|undefined behavior' "$OUT/$label.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan_O2.stdout"

build_run mutant_stride -O2 "" -DWM_74F2C_MUTANT_STRIDE
build_run mutant_trigger -O2 "" -DWM_74F2C_MUTANT_TRIGGER
build_run mutant_signed_index -O2 "" -DWM_74F2C_MUTANT_INDEX_UNSIGNED
build_run mutant_unknown_guard -O2 "" -DWM_74F2C_MUTANT_UNKNOWN_GUARD
for label in mutant_stride mutant_trigger mutant_signed_index mutant_unknown_guard; do
    test "$(<"$OUT/$label.rc")" != 0
done

printf 'W34B34 TEST PASS O0=7/7 O2=7/7 UBSan_O2=7/7 mutants=4/4 detected\n'
