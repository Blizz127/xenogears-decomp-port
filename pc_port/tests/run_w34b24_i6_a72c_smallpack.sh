#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B24_I6_BUILD_DIR:-$ROOT/pc_port/build_native/w34b24_i6}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_97770_TEST_TRACE -DWM_941C4_TEST_TRACE -DWM_94238_TEST_TRACE
      -DWM_8BEC8_TEST_TRACE -DWM_8C1DC_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b24_i6_a72c_smallpack_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_97770.c
     pc_port/src/world_map_helper_941c4.c
     pc_port/src/world_map_helper_94238.c
     pc_port/src/world_map_helper_8bec8.c
     pc_port/src/world_map_helper_8c1dc.c)

check_slice() {
    local skip="$1" count="$2" want="$3" name="$4" got
    got="$(dd if=disc/world_map.bin bs=1 skip=$((skip)) count="$count" status=none | sha256sum | awk '{print $1}')"
    if [[ "$got" != "$want" ]]; then
        echo "ERROR: $name slice SHA mismatch: $got" >&2
        exit 1
    fi
}

require_retail() {
    local fixture="disc/world_map.bin"
    local expected="4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"
    local actual
    if [[ ! -f "$fixture" ]]; then
        echo "ERROR: missing $fixture" >&2
        exit 1
    fi
    if [[ "$(wc -c < "$fixture")" != "180422" ]]; then
        echo "ERROR: $fixture size is not 180422" >&2
        exit 1
    fi
    actual="$(sha256sum "$fixture" | awk '{print $1}')"
    if [[ "$actual" != "$expected" ]]; then
        echo "ERROR: world_map.bin SHA mismatch: $actual" >&2
        exit 1
    fi
    # Slices (VA - 0x8006FAF0).
    check_slice 0x27C80 56  "b63a8eea33461a34875ddf9c13ef5e56b38d9e820ddb5d33dc866c0beca7e571" "0x80097770"
    check_slice 0x246D4 116 "4fdffabe39505c7bc3fae503fae8d66a18f676ea21fbb255e4386affa70e4ce8" "0x800941C4"
    check_slice 0x24748 300 "2f64495cbbe31ab25e3289a0192446ecb860f7959841c5df27770aa4affcaf69" "0x80094238"
    check_slice 0x1C3D8 268 "2fbee93abbb098e824eb3689c4ca60085c27511ce9fb6bf46292733dace260da" "0x8008BEC8"
    check_slice 0x1C6EC 176 "0ba6d1cf994e4d07b8a0b93d0ea694203d09396470f5fb3488987dc1c9438343" "0x8008C1DC"
}

build_and_run() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" 2>"$BUILD_DIR/$name.stderr"
    rg -v 'PSX RAM emulation' "$BUILD_DIR/$name.raw" >"$BUILD_DIR/$name.stdout" || true
}

require_retail

echo "== focused production regimes =="
build_and_run focused_O0 -O0 -g
build_and_run focused_O2 -O2
build_and_run focused_ubsan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

for regime in focused_O0 focused_O2 focused_ubsan; do
    rg -q '^W34B24-I6 8A72C dependency pack focused oracle PASS' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

mutants=(
    WM_97770_MUTANT_WRONG_STRIDE
    WM_97770_MUTANT_WRONG_OCC_OFFSET
    WM_97770_MUTANT_BUSY_POLARITY
    WM_97770_MUTANT_MISSING_MARK
    WM_97770_MUTANT_WRONG_VALUE_WIDTH
    WM_941C4_MUTANT_SWAPPED_RATAN2_ARGS
    WM_941C4_MUTANT_WRONG_ANGLE_BIAS
    WM_941C4_MUTANT_WRONG_ANGLE_MASK
    WM_941C4_MUTANT_MISSING_SIN_NEGATE
    WM_941C4_MUTANT_SWAPPED_TRIG_ORDER
    WM_94238_MUTANT_MISSING_COORD_MASK
    WM_94238_MUTANT_EXCLUSIVE_BOUND
    WM_94238_MUTANT_WRONG_TYPE_CONST
    WM_94238_MUTANT_SWAPPED_GLOBALS
    WM_94238_MUTANT_WRONG_STRIDE
    WM_8BEC8_MUTANT_WRONG_CLOSE_THRESH
    WM_8BEC8_MUTANT_MISSING_ABS
    WM_8BEC8_MUTANT_WRONG_STEP_ROUND
    WM_8BEC8_MUTANT_SWAPPED_AXES
    WM_8BEC8_MUTANT_MASK_POLARITY
    WM_8C1DC_MUTANT_WRONG_BRANCH_CONST
    WM_8C1DC_MUTANT_MISSING_SIGN16
    WM_8C1DC_MUTANT_WRONG_OFFSET_A4
    WM_8C1DC_MUTANT_SWAPPED_CALLEE
    WM_8C1DC_MUTANT_WRONG_5C_WIDTH
)

killed=0
for mutant in "${mutants[@]}"; do
    name="mutant_${mutant}"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 -D"$mutant" "${SRC[@]}" -o "$BUILD_DIR/$name"
    set +e
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr"
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! rg -q '^ASSERTION ' "$BUILD_DIR/$name.stderr"; then
        echo "$mutant FAILED mutant gate (rc=$rc)" >&2
        tail -30 "$BUILD_DIR/$name.stderr" >&2 || true
        exit 1
    fi
    assertion="$(rg -m1 '^ASSERTION ' "$BUILD_DIR/$name.stderr")"
    echo "$mutant KILLED rc=$rc $assertion"
    killed=$((killed+1))
done

echo "MUTANTS ${killed}/${#mutants[@]} KILLED"
echo "W34B24-I6 8A72C dependency pack certificate PASS"
