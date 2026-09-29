#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/archive-file-test.XXXXXX")"
trap 'rm -rf -- "$BUILD_DIR"' EXIT
cd "$ROOT"
CC="${CC:-gcc}"
BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h
      -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
build_case() {
    local name="$1"
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" -w \
        -c src/slus_006.64/system/libarchive.c -o "$BUILD_DIR/$name.archive.o"
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" -Wall -Wextra -Werror \
        pc_port/tests/libarchive_file_integration_test.c pc_port/src/pc_file_io.c \
        "$BUILD_DIR/$name.archive.o" -Wl,--gc-sections -o "$BUILD_DIR/$name"
}
for mode in O0 O2 UBSan; do
    FLAGS=(-O2)
    [[ "$mode" != O0 ]] || FLAGS=(-O0)
    [[ "$mode" != UBSan ]] || FLAGS+=(-fsanitize=undefined -fno-sanitize-recover=all)
    build_case "$mode" "${FLAGS[@]}"
    "$BUILD_DIR/$mode" "$BUILD_DIR"
done
build_case mutant -O0 -DLIBARCHIVE_28F30_MUTANT_PAYLOAD_SHIFT4
if "$BUILD_DIR/mutant" "$BUILD_DIR" >"$BUILD_DIR/mutant.log" 2>&1; then
    echo 'ERROR: payload-offset mutant survived' >&2
    exit 1
fi
grep -q '^ASSERTION archive.file .*memcmp' "$BUILD_DIR/mutant.log"
echo 'ARCHIVE FILE INTEGRATION payload-offset mutant detected'
