#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/vblank-helpers.XXXXXX")"
trap 'rm -rf -- "$BUILD_DIR"' EXIT
GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
      -ffunction-sections -fdata-sections -fno-builtin
      -Ipc_port/include_shim -Iinclude -Ipc_port/src
      -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
# The production bodies: system/controller2.c, one body for both builds.
controller2_obj() {  # $1=out $2..=flags
    local out="$1"; shift
    "${CC:-gcc}" "${GAME[@]}" "$@" -w -c src/slus_006.64/system/controller2.c -o "$out"
}
for mode in O0 O2 UBSan; do
    flags=(-O2)
    [[ "$mode" != O0 ]] || flags=(-O0)
    [[ "$mode" != UBSan ]] || flags+=(-fsanitize=undefined -fno-sanitize-recover=all)
    controller2_obj "$BUILD_DIR/$mode.controller2.o" "${flags[@]}"
    "${CC:-gcc}" -std=c11 -Wall -Wextra -Werror "${flags[@]}" \
        pc_port/tests/controller_vblank_helpers_test.c \
        "$BUILD_DIR/$mode.controller2.o" -Wl,--gc-sections -o "$BUILD_DIR/$mode"
    "$BUILD_DIR/$mode"
done
