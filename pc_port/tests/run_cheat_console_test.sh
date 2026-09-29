#!/usr/bin/env bash
# Cheat console on the xg_plat hook layer (no game data needed).
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/cheat_console_test"
mkdir -p "$(dirname "$out")"
gcc -std=gnu17 -O2 -Wall -Wextra -Werror pc_port/tests/cheat_console_test.c \
    pc_port/src/cheat_console.c pc_port/src/plat/xg_plat_mods.c \
    pc_port/src/plat/xg_plat_config.c -ldl -lpthread -o "$out"
"$out"
