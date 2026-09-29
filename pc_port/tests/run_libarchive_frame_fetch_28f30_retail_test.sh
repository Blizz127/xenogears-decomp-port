#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${LIBARCHIVE_FRAME_FETCH_28F30_BUILD_DIR:-$ROOT/pc_port/build_native/libarchive_frame_fetch_28f30}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
FN_SHA="0a66fc9e4c5754e0b71bd94acac4a66b5e7375444eed5622bb9a38068ec4b0da"
actual="$(sha256sum disc/SLUS_006.64 | awk '{print $1}')"
if [[ "$actual" != "$SLUS_SHA" ]]; then
    echo "ERROR: disc/SLUS_006.64 SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/SLUS_006.64 bs=1 \
    skip=$((0x800 + 0x80028F30 - 0x80010000)) count=$((0x52C)) status=none \
    | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$FN_SHA" ]]; then
    echo "ERROR: retail func_80028F30 slice SHA-256 mismatch: $actual" >&2
    exit 1
fi

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

build_and_run() {
    local name="$1"
    local linker="$CC"
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" -w -fno-inline "$@" \
        -fno-sanitize=object-size \
        -c src/slus_006.64/system/libarchive.c -o "$BUILD_DIR/$name.libarchive.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/libarchive_frame_fetch_28f30_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -fno-pie -no-pie "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.libarchive.o" \
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
grep -Eq '^LIBARCHIVE FRAME FETCH 28F30 certificate PASS checks=[0-9]+$' \
    "$BUILD_DIR/O0.stdout"

echo "LIBARCHIVE FRAME FETCH 28F30 O0/O2/UBSAN PASS"

set +e
build_and_run mutant_payload_shift4 -O0 -g \
    -DLIBARCHIVE_28F30_MUTANT_PAYLOAD_SHIFT4
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! grep -q '^ASSERTION libarchive.frame ' \
       "$BUILD_DIR/mutant_payload_shift4.stderr"; then
    echo "PAYLOAD-SHIFT4 MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_payload_shift4.stderr" >&2 || true
    exit 1
fi

echo "LIBARCHIVE FRAME FETCH 28F30 MUTANT DETECTED"
