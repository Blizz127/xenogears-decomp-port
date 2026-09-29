#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B24_I8_BUILD_DIR:-$ROOT/pc_port/build_native/w34b24_i8}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -fno-pie -no-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DWM_8A72C_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b24_i8_8a72c_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_callback_8a72c.c)

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
    if [[ "$(stat -Lc '%s' "$fixture")" != "180422" ]]; then
        echo "ERROR: $fixture size is not 180422" >&2
        exit 1
    fi
    actual="$(sha256sum "$fixture" | awk '{print $1}')"
    if [[ "$actual" != "$expected" ]]; then
        echo "ERROR: world_map.bin SHA mismatch: $actual" >&2
        exit 1
    fi
    # wm_8008A72C body (VA - 0x8006FAF0 = 0x1AC3C).
    check_slice 0x1AC3C 2960 "1d58efac94432cb6892462260a1d7679dfcfddc9d0568b08244b1fd7cf1b4b3f" "0x8008A72C"
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
    rg -q '^W34B24-I8 0x8008A72C focused oracle PASS' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

mutants=(
    WM_ANIMATION_GUARD_MUTANT_GUEST_REMAP
    WM_8A72C_MUTANT_SUBSTATE_POLARITY
    WM_8A72C_MUTANT_LAP_GATE
    WM_8A72C_MUTANT_JT1_GUARD
    WM_8A72C_MUTANT_JT2_BIAS
    WM_8A72C_MUTANT_JT2_OOR_POLARITY
    WM_8A72C_MUTANT_RETRY_COPY_EXTENT
    WM_8A72C_MUTANT_ZERO_FALLBACK_EXTENT
    WM_8A72C_MUTANT_R_TEST_WIDTH
    WM_8A72C_MUTANT_AREA_PLUS3
    WM_8A72C_MUTANT_RING_MASK
    WM_8A72C_MUTANT_NEIGHBOR_OFFSET
    WM_8A72C_MUTANT_MIRROR_ADDR
    WM_8A72C_MUTANT_RETURN_VARIABLE
    WM_8A72C_MUTANT_74794_GATE
    WM_8A72C_MUTANT_STATE2D_TARGET
    WM_8A72C_MUTANT_COMMIT_SET
    WM_8A72C_MUTANT_97770_ARGS
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
echo "W34B24-I8 0x8008A72C focused certificate PASS"
