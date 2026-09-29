#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${W34B30_OUT:-pc_port/build_native/w34b30-cert}"
mkdir -p "$OUT"
FLAGS=(
    -std=gnu17 -g -Wall -Wextra -Wconversion -Wsign-conversion -Werror
    -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
)
TEST="pc_port/tests/w34b30_8007197c_image_transfer_prod_test.c"
TRANSFER="pc_port/src/world_map_image_transfer_25044.c"

build_run() {
    local label="$1" opt="$2" san="${3:-}"
    gcc -c "$TEST" "${FLAGS[@]}" "$opt" $san -o "$OUT/$label.test.o"
    gcc -c "$TRANSFER" "${FLAGS[@]}" "$opt" $san \
        -o "$OUT/$label.transfer.o"
    gcc -no-pie -Wl,--gc-sections $san "$OUT/$label.test.o" \
        "$OUT/$label.transfer.o" -o "$OUT/$label"
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
    rg -q '^=== Results: 8/8 PASS ===$' "$OUT/$label.stdout"
    ! rg -qi 'runtime error|undefined behavior' "$OUT/$label.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan_O2.stdout"
printf 'W34B30 TEST PASS O0=8/8 O2=8/8 UBSan_O2=8/8\n'
