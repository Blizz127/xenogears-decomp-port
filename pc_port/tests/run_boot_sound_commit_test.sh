#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${XENO_BOOT_SOUND_COMMIT_BUILD_DIR:-$ROOT/pc_port/build_native/boot_sound_commit}"
CC_BIN="${CC:-cc}"

mkdir -p "$BUILD_DIR"

build_and_run() {
    local mode="$1"
    shift
    "$CC_BIN" -std=gnu17 -include assert.h -Wall -Wextra -Werror \
        -m64 -fno-builtin \
        -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -I"$ROOT/include" -I"$ROOT/pc_port/src" \
        "$@" \
        "$ROOT/pc_port/tests/boot_sound_commit_test.c" \
        "$ROOT/pc_port/src/boot_sound_commit.c" \
        -o "$BUILD_DIR/test-$mode"
    "$BUILD_DIR/test-$mode"
}

build_and_run O0 -O0
build_and_run O2 -O2
build_and_run UBSan -O1 -fsanitize=undefined -fno-sanitize-recover=all
