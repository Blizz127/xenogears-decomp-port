#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${LIBARCHIVE_STREAM_POLL_28B14_BUILD_DIR:-$ROOT/pc_port/build_native/libarchive_stream_poll_28b14}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
FN_SHA="67a60f31a93538187696cf11da8f682b75296b6fd4eac252a213254c693583a4"
actual="$(sha256sum disc/SLUS_006.64 | awk '{print $1}')"
if [[ "$actual" != "$SLUS_SHA" ]]; then
    echo "ERROR: disc/SLUS_006.64 SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/SLUS_006.64 bs=1 \
    skip=$((0x800 + 0x80028B14 - 0x80010000)) count=$((0x34C)) status=none \
    | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$FN_SHA" ]]; then
    echo "ERROR: retail func_80028B14 slice SHA-256 mismatch: $actual" >&2
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
        -fno-sanitize=object-size \
        -c src/slus_006.64/system/libarchive.c -o "$BUILD_DIR/$name.libarchive.o"
    objcopy --weaken-symbol=ArchiveDecodeSizeAligned \
            --weaken-symbol=ArchiveGetFilePath \
            "$BUILD_DIR/$name.libarchive.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/libarchive_stream_poll_28b14_retail_test.c \
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
rg -q '^LIBARCHIVE STREAM POLL 28B14 certificate PASS checks=[0-9]+$' \
    "$BUILD_DIR/O0.stdout"

echo "LIBARCHIVE STREAM POLL 28B14 O0/O2/UBSAN PASS"

set +e
build_and_run mutant_mark_state2 -O0 -g \
    -DLIBARCHIVE_28B14_MUTANT_MARK_STATE2
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION libarchive.poll ' \
       "$BUILD_DIR/mutant_mark_state2.stderr"; then
    echo "MARK-STATE2 MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_mark_state2.stderr" >&2 || true
    exit 1
fi

echo "LIBARCHIVE STREAM POLL 28B14 MUTANT DETECTED"
