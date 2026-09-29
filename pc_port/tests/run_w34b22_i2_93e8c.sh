#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B22_I2_BUILD_DIR:-$ROOT/pc_port/build_native/w34b22_i2}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DWM_93E8C_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b22_i2_93e8c_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_helper_93e8c.c)

require_retail() {
    local fixture="disc/world_map.bin"
    local expected="4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"
    local slice_expected="de900c3caeafff6767250bcfee56252778b0999805f1c6a4f3ec33367fea0e13"
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
        echo "ERROR: world_map.bin SHA-256 mismatch: $actual" >&2
        exit 1
    fi
    actual="$(dd if="$fixture" bs=1 skip=$((0x80093E8C - 0x8006FAF0)) \
        count=140 status=none | sha256sum | awk '{print $1}')"
    if [[ "$actual" != "$slice_expected" ]]; then
        echo "ERROR: 0x80093E8C slice SHA-256 mismatch: $actual" >&2
        exit 1
    fi
}

build_and_run() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" \
        2>"$BUILD_DIR/$name.stderr"
    rg -v 'PSX RAM emulation' "$BUILD_DIR/$name.raw" >"$BUILD_DIR/$name.stdout"
}

require_retail

echo "== focused production regimes =="
build_and_run focused_O0 -O0 -g
build_and_run focused_O2 -O2
build_and_run focused_ubsan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

for regime in focused_O0 focused_O2 focused_ubsan; do
    rg -q '^W34B22-I2 0x80093E8C focused oracle PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

mutants=(WRONG_Z_OFFSET WRONG_X_OFFSET WRONG_SHIFT_20 SKIP_NEG_ADJUST
         WRONG_STRIDE_GLOBAL WRONG_TABLE_BASE WRONG_RECORD_OFFSET
         LOAD_U32 LOAD_U16 UNSIGNED_SHIFT WRONG_MASK WRONG_PACK
         SKIP_S16_INDEX BOOLEAN_RETURN WRONG_PACK_SHIFT)
killed=0
for mutant in "${mutants[@]}"; do
    name="mutant_${mutant}"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 \
        -DWM_93E8C_MUTANT_"$mutant" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    set +e
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! rg -q '^ASSERTION ' "$BUILD_DIR/$name.stderr"; then
        echo "$mutant FAILED mutant gate (rc=$rc)" >&2
        tail -20 "$BUILD_DIR/$name.stderr" >&2 || true
        exit 1
    fi
    assertion="$(rg -m1 '^ASSERTION ' "$BUILD_DIR/$name.stderr")"
    echo "$mutant compile PASS; execute rc=$rc; $assertion"
    killed=$((killed + 1))
done

echo "MUTANTS ${killed}/${#mutants[@]} KILLED"
echo "W34B22-I2 0x80093E8C focused certificate PASS"
