#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
PSYX="pc_port/extern/PsyCross"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xeno-vsync-regression.XXXXXX")"
trap 'rm -rf "$BUILD_DIR"' EXIT

INC=(-Ipc_port/include_shim -Iinclude -I"$PSYX/include" -I"$PSYX/include/psx")
GFLAGS=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY
        -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
        -include assert.h -w -O0 -g -m64 -fno-builtin
        -ffunction-sections -fdata-sections)

"$CC" -c pc_port/src/psyq_compat.c "${GFLAGS[@]}" "${INC[@]}" \
    -o "$BUILD_DIR/psyq_compat.o"
read -r -a SDL_CFLAGS <<< "$(pkg-config --cflags sdl2)"
"${CXX:-g++}" -std=c++17 -O0 -g -fpermissive -w \
    -ffunction-sections -fdata-sections "${SDL_CFLAGS[@]}" "${INC[@]}" \
    -c "$PSYX/src/psx/LIBAPI.C" -o "$BUILD_DIR/libapi.o"
"$CC" "${GFLAGS[@]}" "${INC[@]}" \
    -c pc_port/tests/vsync_present_regression_test.c \
    -o "$BUILD_DIR/harness.o"
# Controller vblank bodies: system/controller2.c (one body for both builds).
"$CC" -c src/slus_006.64/system/controller2.c "${GFLAGS[@]}" "${INC[@]}" -Ipc_port/src \
    -o "$BUILD_DIR/controller2.o"
"$CC" "${GFLAGS[@]}" "${INC[@]}" -Wl,--gc-sections \
    pc_port/src/controller_vblank_service.c "$BUILD_DIR/controller2.o" \
    "$BUILD_DIR/harness.o" "$BUILD_DIR/psyq_compat.o" "$BUILD_DIR/libapi.o" \
    -o "$BUILD_DIR/test_vsync"

env -u XENO_MENU_FORCE -u XENO_MENU_NAV_TEST -u XENO_KERNEL_SEL \
    -u XENO_FIELD_TEST -u XENO_MENU_FORCE_DELAY -u XENO_KERNEL_DELAY \
    -u XENO_FIELD_CAPTURE_DIR -u XENO_CULL_CAM_LOG \
    "$BUILD_DIR/test_vsync"
