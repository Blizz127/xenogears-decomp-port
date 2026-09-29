#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${W34B42_OUT:-pc_port/build_native/w34b42-cert}"
mkdir -p "$OUT"
FLAGS=(
    -std=gnu17 -g -Wall -Wextra -Wconversion -Wsign-conversion -Werror
    -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
)
TEST="pc_port/tests/w34b42_frame_reentry_predicate_prod_test.c"
DRIVER="pc_port/src/world_map_frame_driver.c"

build_run() {
    local label="$1" opt="$2" san="${3:-}" defs="${4:-}"
    gcc -c "$TEST" "${FLAGS[@]}" "$opt" $san $defs -o "$OUT/$label.test.o"
    gcc -c "$DRIVER" "${FLAGS[@]}" "$opt" $san $defs -o "$OUT/$label.driver.o"
    gcc -no-pie -Wl,--gc-sections $san "$OUT/$label.test.o" \
        "$OUT/$label.driver.o" -o "$OUT/$label"
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
    rg -q '^=== Results: 5/5 PASS ===$' "$OUT/$label.stdout"
    ! rg -qi 'runtime error|undefined behavior' "$OUT/$label.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan_O2.stdout"

build_run mutant_gate -O2 "" -DWM_W34B42_MUTANT_GATE
test "$(<"$OUT/mutant_gate.rc")" != 0
build_run mutant_d554 -O2 "" -DWM_W34B42_MUTANT_D554
test "$(<"$OUT/mutant_d554.rc")" != 0
build_run mutant_always -O2 "" -DWM_W34B42_MUTANT_ALWAYS
test "$(<"$OUT/mutant_always.rc")" != 0

printf 'W34B42 TEST PASS O0=5/5 O2=5/5 UBSan_O2=5/5 mutants=3/3 DETECTED\n'
