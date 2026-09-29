#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_FE60_TRANSITION_A7C58_BUILD_DIR:-$ROOT/pc_port/build_native/field_fe60_transition_a7c58}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

FIELD_SHA="38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"
FN_SHA="50c91b4a4b7ed0bef9cd98471f69ca35c87ae79a6c4d1cd9095f912f47eeb319"
actual="$(sha256sum disc/field.bin | awk '{print $1}')"
if [[ "$actual" != "$FIELD_SHA" ]]; then
    echo "ERROR: disc/field.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/field.bin bs=1 skip=$((0x800A7C58 - 0x8006FAF0)) \
    count=$((0x6BC)) status=none | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$FN_SHA" ]]; then
    echo "ERROR: retail func_800A7C58 slice SHA-256 mismatch: $actual" >&2
    exit 1
fi

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

build_and_run() {
    local name="$1"
    local linker="$CC"
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" -w -fno-inline "$@" \
        -c src/field/main/misc5.c -o "$BUILD_DIR/$name.misc5.o"
    objcopy --weaken-symbol=func_800A732C \
            --weaken-symbol=func_800A7394 \
            --weaken-symbol=func_800A73E8 \
            --weaken-symbol=func_800A708C \
            --weaken-symbol=func_800A7218 \
            --weaken-symbol=func_800A7948 \
            --weaken-symbol=func_800A74F8 \
            --weaken-symbol=FieldImageConvert24BitTo15Bit \
            "$BUILD_DIR/$name.misc5.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/field_fe60_transition_a7c58_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -fno-pie -no-pie "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.misc5.o" \
        -Wl,--gc-sections -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
rg -q '^FIELD FE60 TRANSITION A7C58 certificate PASS checks=54$' \
    "$BUILD_DIR/O0.stdout"

echo "FIELD FE60 TRANSITION A7C58 O0/O2/UBSAN PASS"

set +e
build_and_run mutant_move_dst0 -O0 -g \
    -DFIELD_A7C58_MUTANT_MOVE_DST0
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION fe60.trans ' \
       "$BUILD_DIR/mutant_move_dst0.stderr"; then
    echo "MOVE-DST0 MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_move_dst0.stderr" >&2 || true
    exit 1
fi

echo "FIELD FE60 TRANSITION A7C58 MUTANT DETECTED"
