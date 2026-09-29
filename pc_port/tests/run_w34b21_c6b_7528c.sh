#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B21_C6B_BUILD_DIR:-$ROOT/pc_port/build_native/w34b21_c6b}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b21_c6b_7528c_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_helper_7528c.c)

build_and_run() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
}

build_and_run focused_O0 -O0 -g
build_and_run focused_O2 -O2
build_and_run focused_ubsan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

for regime in focused_O0 focused_O2 focused_ubsan; do
    rg -q '^W34B21-C6B 0x8007528C focused oracle PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

for mutant in $(seq 1 14); do
    name="mutant_M$mutant"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 \
        -DWM_7528C_MUTANT_M"$mutant" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    set +e
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! rg -q '^ASSERTION ' "$BUILD_DIR/$name.stderr"; then
        echo "M$mutant FAILED mutant gate (rc=$rc)" >&2
        exit 1
    fi
    assertion="$(rg -m1 '^ASSERTION ' "$BUILD_DIR/$name.stderr")"
    echo "M$mutant compile PASS; execute rc=$rc; $assertion"
done

echo "MUTANTS 14/14 KILLED"
echo "W34B21-C6B 0x8007528C focused certificate PASS"
