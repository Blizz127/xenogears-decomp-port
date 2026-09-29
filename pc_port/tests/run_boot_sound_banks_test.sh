#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${XENO_BOOT_SOUND_BUILD_DIR:-$ROOT/pc_port/build_native/boot_sound_banks}"
CC_BIN="${CC:-cc}"

mkdir -p "$BUILD_DIR"

build_and_run() {
    local mode="$1"
    shift
    "$CC_BIN" -std=gnu17 -fpermissive -w -m64 \
        -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -I"$ROOT/include" -I"$ROOT/pc_port/src" \
        "$@" \
        "$ROOT/pc_port/tests/boot_sound_banks_test.c" \
        "$ROOT/pc_port/src/boot_sound_banks.c" \
        -o "$BUILD_DIR/test-$mode"
    "$BUILD_DIR/test-$mode"
}

build_and_run O0 -O0
build_and_run O2 -O2
build_and_run UBSan -O1 -fsanitize=undefined -fno-sanitize-recover=all

load_sha=$(dd if="$ROOT/disc/SLUS_006.64" bs=1 \
    skip=$((0x800 + 0x80019670 - 0x80010000)) count=$((0x104)) status=none |
    sha256sum | awk '{print $1}')
drain_sha=$(dd if="$ROOT/disc/SLUS_006.64" bs=1 \
    skip=$((0x800 + 0x80019840 - 0x80010000)) count=$((0x28)) status=none |
    sha256sum | awk '{print $1}')
test "$load_sha" = 1894a377dcc4940bcc83fbd1c926acc94af3ae6b26c4574f1d08579400d2c410
test "$drain_sha" = 0f1c263653017d52e9d36479814c20d09a3ccca8a2554f576adf1ca3fa2c2c78

echo "boot sound banks retail bytes: load=$load_sha drain=$drain_sha"
