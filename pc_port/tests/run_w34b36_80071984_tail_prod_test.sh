#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${W34B36_OUT:-pc_port/build_native/w34b36-cert}"
mkdir -p "$OUT"
FLAGS=(
    -std=gnu17 -g -Wall -Wextra -Wconversion -Wsign-conversion -Werror
    -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
)
TEST="pc_port/tests/w34b36_80071984_tail_prod_test.c"
TAIL="pc_port/src/world_map_frame_tail_71984.c"

build_run() {
    local label="$1" opt="$2" san="${3:-}" defs="${4:-}"
    gcc -c "$TEST" "${FLAGS[@]}" "$opt" $san $defs -o "$OUT/$label.test.o"
    gcc -c "$TAIL" "${FLAGS[@]}" "$opt" $san $defs -o "$OUT/$label.tail.o"
    gcc -c pc_port/src/world_map_ot_adapter.c -std=gnu17 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src "$opt" $san $defs -o "$OUT/$label.ot.o"
    gcc -no-pie -Wl,--gc-sections $san "$OUT/$label.test.o" \
        "$OUT/$label.tail.o" "$OUT/$label.ot.o" -o "$OUT/$label"
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
    rg -q '^=== Results: 4/4 PASS ===$' "$OUT/$label.stdout"
    ! rg -qi 'runtime error|undefined behavior' "$OUT/$label.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan_O2.stdout"

build_run mutant_unknown_guard -O2 "" -DWM_71984_MUTANT_UNKNOWN_GUARD
test "$(<"$OUT/mutant_unknown_guard.rc")" != 0

printf 'W34B36 TEST PASS O0=4/4 O2=4/4 UBSan_O2=4/4 unknown-guard mutant=detected\n'
