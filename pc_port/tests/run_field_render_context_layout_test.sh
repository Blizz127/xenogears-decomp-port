#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PSYX="$ROOT/pc_port/extern/PsyCross"
BUILD_DIR="${XENO_FIELD_RENDER_LAYOUT_BUILD_DIR:-$ROOT/pc_port/build_native/field_render_context_layout}"
CC_BIN="${CC:-cc}"

mkdir -p "$BUILD_DIR"

"$CC_BIN" -std=gnu17 -include assert.h -Wall -Wextra -Werror \
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
    -I"$ROOT/pc_port/include_shim" -I"$ROOT/include" \
    -I"$PSYX/include" -I"$PSYX/include/psx" \
    "$ROOT/pc_port/tests/field_render_context_layout_test.c" \
    -o "$BUILD_DIR/test"

"$BUILD_DIR/test"
