#!/usr/bin/env bash
# W34N131: the four world-map private collision probes against the retail instructions.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d /tmp/w34n131.XXXXXX)
trap 'rm -rf "$OUT"' EXIT
for mode in O2 UBSan; do
    flags=(-O2); [ "$mode" = UBSan ] && flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    gcc -std=gnu17 "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DWM_PRIVATE_PROBE_TEST_TRACE \
        -Wall -Wextra -Werror -Wno-misleading-indentation -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        pc_port/tests/w34n131_private_probe_retail_diff_test.c pc_port/src/world_map_private_collision.c \
        pc_port/src/battle_mips_adapter.c -o "$OUT/t_$mode"
    "$OUT/t_$mode"
done
echo "W34N131 PRIVATE PROBE RETAIL DIFF PASS"
