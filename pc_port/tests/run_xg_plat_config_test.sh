#!/usr/bin/env bash
# xg_plat config parser and XENO_* overrides (no game data needed).
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/xg_plat_config_test"
mkdir -p "$(dirname "$out")"
gcc -std=gnu17 -O2 -Wall -Wextra -Werror pc_port/tests/xg_plat_config_test.c \
    pc_port/src/plat/xg_plat_config.c -o "$out"
"$out"
