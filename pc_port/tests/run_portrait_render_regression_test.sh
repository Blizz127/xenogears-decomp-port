#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
PSYX="pc_port/extern/PsyCross"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xeno-portrait-regression.XXXXXX")"
trap 'rm -rf "$BUILD_DIR"' EXIT

INC=(-Ipc_port/include_shim -Iinclude -I"$PSYX/include" -I"$PSYX/include/psx")
GFLAGS=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY
        -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
        -include assert.h -w "-${PORTRAIT_OPT:-O0}" -g -m64 -fno-builtin
        -ffunction-sections -fdata-sections)

"$CC" -c src/field/dialogue/text_box_render.c "${GFLAGS[@]}" "${INC[@]}" \
    -o "$BUILD_DIR/text_box_render.o"
"$CC" -c pc_port/tests/portrait_render_regression_test.c \
    "${GFLAGS[@]}" "${INC[@]}" -o "$BUILD_DIR/harness.o"
sed -n -e '/^u_short GetTPage(/,/^}/p' -e '/^void SetDrawMode(/,/^}/p' \
    "$PSYX/src/psx/LIBGPU.C" > "$BUILD_DIR/gpu.c"
"$CC" -c "$BUILD_DIR/gpu.c" "${GFLAGS[@]}" "${INC[@]}" \
    -include common.h -include psyq/libgpu.h -o "$BUILD_DIR/gpu.o"
"$CC" -no-pie -Wl,--gc-sections "$BUILD_DIR/harness.o" "$BUILD_DIR/text_box_render.o" \
    "$BUILD_DIR/gpu.o" -o "$BUILD_DIR/test_portrait"

"$BUILD_DIR/test_portrait"
