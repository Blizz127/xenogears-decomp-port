#!/usr/bin/env bash
# World-map g_GameState fields: saved world position and field transition
# tuple must reach the host g_GameState (world_map_gamestate.h).
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/world_map_gamestate_sync_test"
mkdir -p "$(dirname "$out")"
for opt in -O0 -O2 "-O1 -fsanitize=undefined -fno-sanitize-recover=all"; do
    # shellcheck disable=SC2086
    cc -std=gnu17 $opt -Wall -Wextra -Werror -DXENO_PC_PORT -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        pc_port/tests/world_map_gamestate_sync_test.c pc_port/src/world_map_helper_8e034.c \
        pc_port/src/world_map_helper_8dff4.c -o "$out"
    "$out"
done
