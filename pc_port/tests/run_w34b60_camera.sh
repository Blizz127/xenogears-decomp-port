#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B60_BUILD_DIR:-$ROOT/pc_port/build_native/w34b60_camera}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -no-pie)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
SRC=(pc_port/tests/w34b60_camera_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_97244.c)

compile_and_run() {
    local name="$1"
    local run_rc=0
    shift
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" \
        2>"$BUILD_DIR/$name.stderr" || run_rc=$?
    rg -v 'PSX RAM emulation' "$BUILD_DIR/$name.raw" \
        >"$BUILD_DIR/$name.stdout" || true
    return "$run_rc"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^W34B60 world camera focused certificate PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
echo "CERTIFICATE O0/O2/UBSan PASS; strict warnings clean"

mutants=(
    'M1:WM_CAMERA_MUTANT_CURRENT'
    'M2:WM_CAMERA_MUTANT_NO_ANGLE_NEGATION'
    'M3:WM_CAMERA_MUTANT_WRONG_LOOK_ROWS'
    'M4:WM_CAMERA_MUTANT_NO_TRANSLATION'
    'M5:WM_CAMERA_MUTANT_WRONG_ANGLE_SOURCE'
)
for entry in "${mutants[@]}"; do
    label="${entry%%:*}"
    define="${entry#*:}"
    set +e
    compile_and_run "$label" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! rg -q '^ASSERTION ' "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED mutant gate rc=$rc" >&2
        exit 1
    fi
    echo "$label DETECTED"
done

echo "W34B60 CERTIFICATE PASS; M1-M5 DETECTED"
