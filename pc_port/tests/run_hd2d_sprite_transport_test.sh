#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-cc}" -std=gnu17 -include assert.h -DXENO_PC_PORT -DUSE_EXTENDED_PRIM_POINTERS=0 -D_LANGUAGE_C \
        -Wall -Wextra -Werror "${flags[@]}" -Ipc_port/include_shim -Iinclude \
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
        pc_port/tests/hd2d_sprite_transport_test.c pc_port/src/hd2d/hd2d_sprite_parts.c -o "$out/test"
    mkdir -p "$out/$mode/pc_port/assets/hd2d"
    (cd "$out/$mode" && XENO_HD2D_SPRITE_DIAG=1 "$out/test")
done
