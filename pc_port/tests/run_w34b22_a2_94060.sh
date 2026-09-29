#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B22_A2_BUILD_DIR:-$ROOT/pc_port/build_native/w34b22_a2}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DWM_94060_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b22_a2_94060_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_helper_94060.c)

require_retail() {
    local fixture="disc/world_map.bin"
    local expected="4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"
    local slice_expected="b40724257fa5cd9734805c3d0464b1fef11fc203de9837bcbb380bf95c36f176"
    local table_expected="718350c853084e59899449f30030f8ed59a52a5c7bd59558267558e90287aea2"
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
    actual="$(dd if="$fixture" bs=1 skip=$((0x80094060 - 0x8006FAF0)) \
        count=40 status=none | sha256sum | awk '{print $1}')"
    if [[ "$actual" != "$slice_expected" ]]; then
        echo "ERROR: 0x80094060 slice SHA-256 mismatch: $actual" >&2
        exit 1
    fi
    actual="$(dd if="$fixture" bs=1 skip=$((0x8009BAC8 - 0x8006FAF0)) \
        count=128 status=none | sha256sum | awk '{print $1}')"
    if [[ "$actual" != "$table_expected" ]]; then
        echo "ERROR: 0x8009BAC8 table slice SHA-256 mismatch: $actual" >&2
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
    rg -q '^W34B22-A2 0x80094060 focused oracle PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

mutants=(CLAMP_ADDED SWAPPED_INDICES WRONG_SHIFTS WRONG_TABLE_BASE
         WRONG_WIDTH UNSIGNED_RETURN)
killed=0
for mutant in "${mutants[@]}"; do
    name="mutant_${mutant}"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 \
        -DWM_94060_MUTANT_"$mutant" "${SRC[@]}" \
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

echo "== natural noninterference =="
gcc -c pc_port/src/world_map_image_transfer_25044.c -std=gnu17 -O2 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -o "$BUILD_DIR/noninterfer.xfer.o"
gcc -c pc_port/src/world_map_upload_pump_74f2c.c -std=gnu17 -O2 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -o "$BUILD_DIR/noninterfer.pump.o"
gcc -c pc_port/src/world_map_ot_adapter.c -std=gnu17 -O2 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -o "$BUILD_DIR/noninterfer.ot.o"
gcc -std=gnu17 -O2 -no-pie -Wall -Wextra -Werror -DXENO_PC_PORT -DSKIP_ASM \
    -D_LANGUAGE_C -Iinclude -Ipc_port/src -Ipc_port/include_shim \
    -DWM_7169C_CONTINUATION_DISABLED -DWM_7197C_CONTINUATION_DISABLED \
    -DWM_71984_CONTINUATION_DISABLED -DWM_71490_CONTINUATION_DISABLED \
    pc_port/tests/w34b22_a2_94060_noninterfer_test.c \
    pc_port/src/world_map_helper_94060.c \
    pc_port/src/world_map_scheduler.c \
    pc_port/src/world_map_frame_driver.c \
    pc_port/src/psx_memory.c \
    "$BUILD_DIR/noninterfer.xfer.o" "$BUILD_DIR/noninterfer.pump.o" "$BUILD_DIR/noninterfer.ot.o" \
    -Wl,--gc-sections \
    -o "$BUILD_DIR/noninterfer"
"$BUILD_DIR/noninterfer" >"$BUILD_DIR/noninterfer.stdout" \
    2>"$BUILD_DIR/noninterfer.stderr"
rg -q '^NONINTERFER PASS exec=0 cb=0x8008A72C frame=0x80071490$' \
    "$BUILD_DIR/noninterfer.stdout"

echo "W34B22-A2 0x80094060 focused certificate PASS"
