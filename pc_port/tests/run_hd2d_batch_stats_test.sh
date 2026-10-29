#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-cc}" -std=c17 -Wall -Wextra -Werror "${flags[@]}" -Ipc_port/include_shim \
        pc_port/src/hd2d/hd2d_batch_stats.c pc_port/tests/hd2d_batch_stats_test.c -o "$out/test"
    "$out/test" >"$out/off.log" 2>&1
    test ! -s "$out/off.log"
    "$out/test" active >"$out/on.log" 2>&1
    rg -q 'summary batches=3 peak_splits=4095 array_limit=4096 usable_slots=4095 peak_vertices=1600 capacity_rejections=1' "$out/on.log"
done
