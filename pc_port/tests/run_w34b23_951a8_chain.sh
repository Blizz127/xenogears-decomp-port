#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B23_BUILD_DIR:-$ROOT/pc_port/build_native/w34b23_951a8}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_93FE4_TEST_TRACE -DWM_94088_TEST_TRACE -DWM_951A8_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b23_951a8_chain_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_93e8c.c
     pc_port/src/world_map_helper_93fe4.c
     pc_port/src/world_map_helper_94088.c
     pc_port/src/world_map_helper_951a8.c)

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
    check_slice 0x244F4 32  "e85b0a8ae1bac650bebd93cd70d115ae0ab89281a2eab7fefc0042174cd53991" "0x80093FE4"
    check_slice 0x24598 204 "62ca577ffd7b7d8a0b31b4466a34abfb45f0178bd067d7f3a88af65693013af7" "0x80094088"
    check_slice 0x256B8 264 "bb71acea957b2d9dbe275cc532679d08e60d48bde4d62f072247af90ba4f86dd" "0x800951A8"
}

check_slice() {
    local skip="$1" count="$2" want="$3" name="$4" got
    got="$(dd if=disc/world_map.bin bs=1 skip=$((skip)) count="$count" status=none | sha256sum | awk '{print $1}')"
    if [[ "$got" != "$want" ]]; then
        echo "ERROR: $name slice SHA mismatch: $got" >&2
        exit 1
    fi
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
    rg -q '^W34B23 0x800951A8 chain focused oracle PASS' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

mutants=(
    WM_93FE4_MUTANT_WRONG_MASK
    WM_93FE4_MUTANT_MISSING_MASK
    WM_94088_MUTANT_WRONG_TABLE_BASE
    WM_94088_MUTANT_WRONG_SHIFT
    WM_94088_MUTANT_SWAPPED_AXES
    WM_94088_MUTANT_WRONG_BRANCH_POLARITY
    WM_94088_MUTANT_MISSING_Z_NEGATE
    WM_94088_MUTANT_ZERO_PATH_RETURN
    WM_951A8_MUTANT_WRONG_WORKSPACE
    WM_951A8_MUTANT_MODE_ZERO_EXT
    WM_951A8_MUTANT_WRONG_88_BRANCH
    WM_951A8_MUTANT_SUCCESS_RETURN
    WM_951A8_MUTANT_SIGN_POLARITY
    WM_951A8_MUTANT_CASE3_READS_X
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
echo "W34B23 0x800951A8 chain focused certificate PASS"
