#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B24_I2_BUILD_DIR:-$ROOT/pc_port/build_native/w34b24_i2}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DWM_85158_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b24_i2_85158_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_plane_solver.c
     pc_port/src/world_map_helper_85158.c)

require_retail() {
    local fixture="disc/world_map.bin"
    local expected="4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"
    local slice="1be50c96bf66f3eb63e8c54f49ccedf8cdfaec408d5e7e029503519303d0b4aa"
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
    actual="$(dd if="$fixture" bs=1 skip=$((0x80085158 - 0x8006FAF0)) count=704 status=none | sha256sum | awk '{print $1}')"
    if [[ "$actual" != "$slice" ]]; then
        echo "ERROR: 0x80085158 slice SHA mismatch: $actual" >&2
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
    rg -q '^W34B24-I2 0x80085158 focused oracle PASS' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

mutants=(
    WM_85158_MUTANT_WRONG_ATTR_STRIDE
    WM_85158_MUTANT_MISSING_ATTR_MASK
    WM_85158_MUTANT_WRONG_XZ_ASYMMETRY
    WM_85158_MUTANT_WRONG_SRA
    WM_85158_MUTANT_NODE_ID_SIGN
    WM_85158_MUTANT_VTX_INDEX_UNSIGNED
    WM_85158_MUTANT_WRONG_TRANS_SLOT
    WM_85158_MUTANT_WRONG_SCALE_VALUE
    WM_85158_MUTANT_WRONG_CALL_ORDER
    WM_85158_MUTANT_SWAPPED_OP0_ARGS
    WM_85158_MUTANT_MISSING_SRA2
    WM_85158_MUTANT_WRONG_935DC_ARGS
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
echo "W34B24-I2 0x80085158 focused certificate PASS"
