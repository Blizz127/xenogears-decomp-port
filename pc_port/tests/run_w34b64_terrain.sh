#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B64_BUILD_DIR:-$ROOT/pc_port/build_native/w34b64_terrain}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -no-pie)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

compile_and_run() {
    local component="$1"
    local define="$2"
    local source="$3"
    local regime="$4"
    shift 4
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -D"$define" "$@" \
        pc_port/tests/w34b64_terrain_prod_test.c pc_port/src/psx_memory.c \
        "$source" -o "$BUILD_DIR/${component}_${regime}"
    "$BUILD_DIR/${component}_${regime}" \
        >"$BUILD_DIR/${component}_${regime}.raw" \
        2>"$BUILD_DIR/${component}_${regime}.stderr"
    rg -v 'PSX RAM emulation' "$BUILD_DIR/${component}_${regime}.raw" \
        >"$BUILD_DIR/${component}_${regime}.stdout" || true
    test ! -s "$BUILD_DIR/${component}_${regime}.stderr"
}

for component in dispatch grid submit; do
    if [[ "$component" == dispatch ]]; then
        define=W34B64_DISPATCH_TEST
        source=pc_port/src/world_map_helper_9932c.c
        expected='W34B64 terrain dispatcher certificate PASS'
    elif [[ "$component" == grid ]]; then
        define=W34B64_GRID_TEST
        source=pc_port/src/world_map_helper_99708.c
        expected='W34B64 terrain grid certificate PASS'
    else
        define=W34B64_SUBMIT_TEST
        source=pc_port/src/world_map_helper_9980c.c
        expected='W34B64 terrain submitter certificate PASS'
    fi
    compile_and_run "$component" "$define" "$source" O0 -O0 -g
    compile_and_run "$component" "$define" "$source" O2 -O2
    compile_and_run "$component" "$define" "$source" UBSan -O2 -g \
        -fsanitize=undefined -fno-sanitize-recover=all
    for regime in O0 O2 UBSan; do
        rg -q "^${expected}$" "$BUILD_DIR/${component}_${regime}.stdout"
    done
    cmp "$BUILD_DIR/${component}_O0.stdout" "$BUILD_DIR/${component}_O2.stdout"
    cmp "$BUILD_DIR/${component}_O0.stdout" "$BUILD_DIR/${component}_UBSan.stdout"
done

# W34C6 mutant: the pre-W34C6 grid took the base height from the HIGH byte of
# the packed word (retail 0x80099790-94 uses the low byte). Must be detected.
"$CC" "${BASE[@]}" "${INC[@]}" -w -DW34B64_GRID_TEST -DWM_99708_MUTANT_HIGH_BYTE -O0 \
    pc_port/tests/w34b64_terrain_prod_test.c pc_port/src/psx_memory.c \
    pc_port/src/world_map_helper_99708.c -o "$BUILD_DIR/grid_mutant_high_byte"
set +e
"$BUILD_DIR/grid_mutant_high_byte" >"$BUILD_DIR/grid_mutant_high_byte.raw" 2>&1
rc=$?
set -e
if [[ "$rc" -eq 0 ]] || ! rg -q 'grid Y follows packed height contract' "$BUILD_DIR/grid_mutant_high_byte.raw"; then
    echo "grid mutant HIGH_BYTE not detected (rc=$rc)" >&2
    exit 1
fi
echo "grid mutant HIGH_BYTE: DETECTED (rc=$rc)"

# W34C7 mutant: the pre-W34C7 submitter passed triangle A's V1/V2 swapped
# (retail 0x800998D8-0x8009990C). Must be detected by the vertex-order check.
"$CC" "${BASE[@]}" "${INC[@]}" -w -DW34B64_SUBMIT_TEST -DWM_9980C_MUTANT_SWAPPED_FIRST -O0 \
    pc_port/tests/w34b64_terrain_prod_test.c pc_port/src/psx_memory.c \
    pc_port/src/world_map_helper_9980c.c -o "$BUILD_DIR/submit_mutant_swapped_first"
set +e
"$BUILD_DIR/submit_mutant_swapped_first" >"$BUILD_DIR/submit_mutant_swapped_first.raw" 2>&1
rc=$?
set -e
if [[ "$rc" -eq 0 ]] || ! rg -q 'triangle A vertex order' "$BUILD_DIR/submit_mutant_swapped_first.raw"; then
    echo "submit mutant SWAPPED_FIRST not detected (rc=$rc)" >&2
    exit 1
fi
echo "submit mutant SWAPPED_FIRST: DETECTED (rc=$rc)"

echo "W34B64 TERRAIN CERTIFICATE PASS; O0/O2/UBSan; strict warnings clean"
