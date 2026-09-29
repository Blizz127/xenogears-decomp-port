#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B24_I1_BUILD_DIR:-$ROOT/pc_port/build_native/w34b24_i1}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_952B0_TEST_TRACE -DWM_95324_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b24_i1_952b0_95324_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_952b0.c
     pc_port/src/world_map_helper_95324.c)

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
    # Function slices (VA - 0x8006FAF0).
    check_slice 0x257C0 116 "b621ae26b802510c27575f80f5f92243037dff7ef64fb164aea607103057cfac" "0x800952B0"
    check_slice 0x25834 240 "ff48f82c9b8ecf1041fb581fc55db7f1bfbc5bd0935891487e2de5606e1efa36" "0x80095324"
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
    rg -q '^W34B24-I1 0x800952B0 \+ 0x80095324 focused oracle PASS' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

mutants=(
    WM_952B0_MUTANT_WRONG_BRANCH_POLARITY
    WM_952B0_MUTANT_MISSING_NEGATE
    WM_952B0_MUTANT_ZERO_PATH_COPY
    WM_952B0_MUTANT_SKIPPED_Y_STORE
    WM_952B0_MUTANT_SWAPPED_OPERANDS
    WM_952B0_MUTANT_WRONG_WIDTH
    WM_95324_MUTANT_WRONG_DOWN_SIGN
    WM_95324_MUTANT_WRONG_DOWN_SLOT
    WM_95324_MUTANT_SWAPPED_GTE_ORDER
    WM_95324_MUTANT_WRONG_OP12_ARGS
    WM_95324_MUTANT_WRONG_TAN_OFFSET
    WM_95324_MUTANT_WRONG_BRANCH_POLARITY
    WM_95324_MUTANT_MISSING_NEGATE
    WM_95324_MUTANT_SKIPPED_Y_STORE
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
echo "W34B24-I1 focused certificate PASS"
