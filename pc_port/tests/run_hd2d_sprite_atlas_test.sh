#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-cc}" -std=c17 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror "${flags[@]}" \
        -Ipc_port/include_shim pc_port/src/hd2d/hd2d_sprite_atlas.c \
        pc_port/tests/hd2d_sprite_atlas_test.c -o "$out/test"
    "$out/test"
done
