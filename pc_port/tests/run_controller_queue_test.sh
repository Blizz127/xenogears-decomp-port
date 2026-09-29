#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/controller-queue.XXXXXX")"
trap 'rm -rf -- "$BUILD_DIR"' EXIT
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude)
# system/controller2.c (the vblank dispatcher + helpers, one body for both
# builds) needs the PsyCross headers the port build uses.
GAME=("${BASE[@]}" -include assert.h -fno-builtin -Ipc_port/src
      -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-O2)
    [[ "$mode" != O0 ]] || flags=(-O0)
    [[ "$mode" != UBSan ]] || flags+=(-fsanitize=undefined -fno-sanitize-recover=all)
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" -w \
        -c src/slus_006.64/system/controller.c -o "$BUILD_DIR/$mode.controller.o"
    "${CC:-gcc}" "${GAME[@]}" "${flags[@]}" -w \
        -c src/slus_006.64/system/controller2.c -o "$BUILD_DIR/$mode.controller2.o"
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        pc_port/tests/controller_queue_test.c "$BUILD_DIR/$mode.controller.o" \
        "$BUILD_DIR/$mode.controller2.o" -Wl,--gc-sections -o "$BUILD_DIR/$mode"
    "$BUILD_DIR/$mode"
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -DTEST_VBLANK_QUEUE pc_port/tests/controller_queue_test.c \
        "$BUILD_DIR/$mode.controller2.o" \
        "$BUILD_DIR/$mode.controller.o" -Wl,--gc-sections -Wl,--wrap=ControllerPoll \
        -o "$BUILD_DIR/$mode.integration"
    "$BUILD_DIR/$mode.integration"
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -DTEST_VBLANK_QUEUE -DTEST_VBLANK_SERVICE pc_port/tests/controller_queue_test.c \
        "$BUILD_DIR/$mode.controller2.o" \
        pc_port/src/controller_vblank_service.c \
        "$BUILD_DIR/$mode.controller.o" -Wl,--gc-sections -Wl,--wrap=ControllerPoll \
        -o "$BUILD_DIR/$mode.service"
    "$BUILD_DIR/$mode.service"
done
