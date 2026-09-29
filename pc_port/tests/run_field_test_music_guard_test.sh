#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_TEST_MUSIC_GUARD_BUILD_DIR:-$ROOT/pc_port/build_native/field_test_music_guard}"
CC_BIN="${CC:-cc}"

mkdir -p "$BUILD_DIR"
"$CC_BIN" -std=gnu17 -fpermissive -DXENO_PC_PORT \
    -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C \
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -w -O0 -g -m64 \
    -fno-builtin -ffunction-sections -fdata-sections \
    -I"$ROOT/pc_port/include_shim" -I"$ROOT/include" \
    -I"$ROOT/pc_port/extern/PsyCross/include" \
    -I"$ROOT/pc_port/extern/PsyCross/include/psx" \
    -c "$ROOT/src/field/main/misc3.c" \
    -o "$BUILD_DIR/misc3.o"
"$CC_BIN" -std=c17 -Wall -Wextra -Werror \
    "$ROOT/pc_port/tests/field_test_music_guard_test.c" \
    "$BUILD_DIR/misc3.o" -Wl,--gc-sections -o "$BUILD_DIR/test"
"$BUILD_DIR/test"
