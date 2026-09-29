#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${XENO_FIELD_SOUND_TEARDOWN_BUILD_DIR:-$ROOT/pc_port/build_native/field_sound_teardown}"
CC_BIN="${CC:-clang}"
PROD_CC_BIN="${PROD_CC:-gcc}"

mkdir -p "$OUT"
cd "$ROOT"

COMMON=(
    -std=gnu17 -m64 -fno-builtin -fno-pie
    -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h
    -fpermissive -w -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
)

run_regime() {
    local name="$1"
    shift
    "$CC_BIN" "${COMMON[@]}" "$@" \
        -c pc_port/tests/field_sound_teardown_test.c -o "$OUT/$name.test.o"
    "$PROD_CC_BIN" "${COMMON[@]}" -fpermissive -w "$@" \
        -c src/field/main/misc8.c -o "$OUT/$name.prod.o"
    "$CC_BIN" -no-pie "$@" -Wl,--gc-sections \
        "$OUT/$name.test.o" "$OUT/$name.prod.o" -o "$OUT/$name"
    "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
    rg -q '^FIELD SOUND TEARDOWN PASS checks=19$' "$OUT/$name.stdout"
    test ! -s "$OUT/$name.stderr"
}

run_regime O0 -O0
run_regime O2 -O2
run_regime UBSan -O1 -fsanitize=undefined -fno-sanitize-recover=all

python3 pc_port/tests/field_fire_cue_retail_test.py

# Retail field overlay: func_800864F0 is [0x800864F0,0x80086590).
retail_sha="$({
    dd if=disc/field.bin bs=1 skip=$((0x800864f0 - 0x8006faf0)) \
        count=$((0xa0)) status=none
} | sha256sum | awk '{print $1}')"
test "$retail_sha" = "a8e75f3fda43e64fb9bff2c7a6da45dd99ba9e17e60a116daec287633641eedc"

echo "FIELD SOUND TEARDOWN RETAIL SLICE PASS sha256=$retail_sha"
echo "FIELD SOUND TEARDOWN O0/O2/UBSAN PASS"
