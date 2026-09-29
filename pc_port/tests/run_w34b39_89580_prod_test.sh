#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${W34B39_OUT:-pc_port/build_native/w34b39-cert}"
mkdir -p "$OUT"
FLAGS=(
    -std=gnu17 -g -w
    -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
)
TEST="pc_port/tests/w34b39_89580_prod_test.c"
PROD="pc_port/src/world_map_helper_89580.c"

build_run() {
    local label="$1" opt="$2" san="${3:-}" mut="${4:-}"
    gcc -c "$TEST" "${FLAGS[@]}" "$opt" $san -o "$OUT/$label.test.o"
    gcc -c "$PROD" "${FLAGS[@]}" "$opt" $san $mut -o "$OUT/$label.prod.o"
    gcc -no-pie -Wl,--gc-sections $san "$OUT/$label.test.o" \
        "$OUT/$label.prod.o" -o "$OUT/$label"
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
    rg -q '^=== Results: 6/6 PASS ===$' "$OUT/$label.stdout"
    ! rg -qi 'runtime error|undefined behavior' "$OUT/$label.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan_O2.stdout"

DETECTED=0
for mut in WRONG_SLOT_GLOBAL PARTICLE_GLOBAL SLOT_OFFSET; do
    build_run "mut_$mut" -O2 "" "-DWM_89580_MUTANT_$mut"
    if test "$(<"$OUT/mut_$mut.rc")" != 0; then
        DETECTED=$((DETECTED + 1))
        echo "MUTANT $mut DETECTED"
    else
        echo "MUTANT $mut NOT DETECTED"
    fi
done
test "$DETECTED" = 3
printf 'W34B39 TEST PASS O0=6/6 O2=6/6 UBSan_O2=6/6 mutants=3/3 DETECTED\n'
