#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B21_C1B_BUILD_DIR:-$ROOT/pc_port/build_native/w34b21_c1b}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

INC=(-Iinclude -Ipc_port/src)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DWM_85760_TEST_TRACE)
SRC=(pc_port/tests/w34b21_c1b_85760_prod_test.c
     pc_port/src/world_map_helper_85760.c pc_port/src/psx_memory.c)

require_retail() {
    local fixture="disc/world_map.bin"
    local expected="4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"
    local slice_expected="1332a8e87ab36bd5615d9c325076bf0f1864e66d469b507d6d353722cf106f91"
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
    actual="$(dd if="$fixture" bs=1 skip=$((0x80085760 - 0x8006FAF0)) \
        count=1404 status=none | sha256sum | awk '{print $1}')"
    if [[ "$actual" != "$slice_expected" ]]; then
        echo "ERROR: 0x80085760 slice SHA-256 mismatch: $actual" >&2
        exit 1
    fi
}

build_and_run() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" | tee "$BUILD_DIR/$name.raw"
    rg -v 'PSX RAM emulation' "$BUILD_DIR/$name.raw" > "$BUILD_DIR/$name.out"
}

require_retail

echo "== focused production regimes =="
build_and_run helper_85760_O0 -O0 -g
build_and_run helper_85760_O2 -O2
build_and_run helper_85760_ubsan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

cmp "$BUILD_DIR/helper_85760_O0.out" "$BUILD_DIR/helper_85760_O2.out"
cmp "$BUILD_DIR/helper_85760_O0.out" "$BUILD_DIR/helper_85760_ubsan.out"

echo "== required mutants =="
mutants=(WRONG_OPZ BLEZ_VS_BEQZ WRONG_REWRITE SKIP_CLIP2 EXTRA_CLIP
         WRONG_VN_ORDER WRONG_VERTEX WRONG_SCRATCH STORE_U16
         UNSIGNED_SHIFT MISSING_GTE EXTRA_GLOBAL WRONG_STRIDE WRONG_PACK)
for mutant in "${mutants[@]}"; do
    exe="$BUILD_DIR/helper_85760_mutant_${mutant}"
    log="$exe.log"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O2 \
        -DWM_85760_MUTANT_${mutant} "${SRC[@]}" -o "$exe"
    set +e
    "$exe" >"$log" 2>&1
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! rg -q '^ASSERTION ' "$log"; then
        echo "$mutant FAILED mutant gate (rc=$rc)" >&2
        tail -20 "$log" >&2 || true
        exit 1
    fi
    echo "$mutant: rejected by named assertion"
done

echo "== natural noninterference =="
gcc -c pc_port/src/world_map_image_transfer_25044.c -std=gnu17 -O2 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -o "$BUILD_DIR/noninterfer.xfer.o"
gcc -c pc_port/src/world_map_upload_pump_74f2c.c -std=gnu17 -O2 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -o "$BUILD_DIR/noninterfer.pump.o"
gcc -c pc_port/src/world_map_ot_adapter.c -std=gnu17 -O2 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -o "$BUILD_DIR/noninterfer.ot.o"
gcc -std=gnu17 -O2 -no-pie -Wall -Wextra -Werror -DXENO_PC_PORT -DSKIP_ASM \
    -D_LANGUAGE_C -Iinclude -Ipc_port/src \
    -DWM_7169C_CONTINUATION_DISABLED -DWM_7197C_CONTINUATION_DISABLED \
    -DWM_71984_CONTINUATION_DISABLED -DWM_71490_CONTINUATION_DISABLED \
    pc_port/tests/w34b21_c1b_85760_noninterfer_test.c \
    pc_port/src/world_map_helper_85760.c \
    pc_port/src/world_map_scheduler.c \
    pc_port/src/world_map_frame_driver.c \
    pc_port/src/psx_memory.c \
    "$BUILD_DIR/noninterfer.xfer.o" "$BUILD_DIR/noninterfer.pump.o" "$BUILD_DIR/noninterfer.ot.o" \
    -Wl,--gc-sections \
    -o "$BUILD_DIR/helper_85760_noninterfer"
"$BUILD_DIR/helper_85760_noninterfer"

echo "W34B21-C1B 0x80085760 focused certificate PASS"
