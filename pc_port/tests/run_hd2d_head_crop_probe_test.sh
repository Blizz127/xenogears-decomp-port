#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-cc}" -std=c17 -Wall -Wextra -Werror "${flags[@]}" -Ipc_port/include_shim \
        pc_port/tests/hd2d_head_crop_probe.c pc_port/tests/hd2d_head_crop_probe_test.c -o "$out/test"
    XENO_HD2D_HEAD_CROP_VARIANT_LOG="$out/variants-$mode.log" "$out/test"
    test "$(rg -c 'gpu_consumed=1 texture=18' "$out/variants-$mode.log")" = 1
done
