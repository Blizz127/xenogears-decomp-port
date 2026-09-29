#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${W34B38_OUT:-pc_port/build_native/w34b38-cert}"
mkdir -p "$OUT"
FLAGS=(
    -std=gnu17 -g -Wall -Wextra -Wconversion -Wsign-conversion -Werror
    -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
)
TEST="pc_port/tests/w34b38_ot_adapter_prod_test.c"
ADAPTER="pc_port/src/world_map_ot_adapter.c"

build_run() {
    local label="$1" opt="$2" san="${3:-}" mut="${4:-}"
    gcc -c "$TEST" "${FLAGS[@]}" "$opt" $san -o "$OUT/$label.test.o"
    gcc -c "$ADAPTER" "${FLAGS[@]}" "$opt" $san $mut -o "$OUT/$label.adapter.o"
    gcc -no-pie -Wl,--gc-sections $san "$OUT/$label.test.o" \
        "$OUT/$label.adapter.o" -o "$OUT/$label"
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

DETECTED=0
for M in M1 M2 M3 M4; do
    build_run "mut_$M" -O2 "" "-DWM_OTA_MUTANT_$M"
    if test "$(<"$OUT/mut_$M.rc")" != 0; then
        DETECTED=$((DETECTED+1))
        echo "MUTANT $M DETECTED"
    else
        echo "MUTANT $M NOT DETECTED"
    fi
done
test "$DETECTED" = 4
printf 'W34B38 TEST PASS O0=7/7 O2=7/7 UBSan_O2=7/7 mutants=4/4 DETECTED\n'
