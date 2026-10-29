#!/usr/bin/env bash
# input.pad_confirm toggle: applies, is written to the user's config, and
# survives a restart (pc_port/tests/pad_confirm_test.c; no game data).
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/pad-confirm.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
gcc -std=gnu17 -O1 -g -Wall -Wextra -Ipc_port/src pc_port/tests/pad_confirm_test.c \
    pc_port/src/port_pad_confirm.c pc_port/src/plat/xg_plat_config.c -o "$OUT/pad_confirm_test"
export HOME="$OUT/home" XDG_CONFIG_HOME="$OUT/xdg"
mkdir -p "$HOME"
unset XENO_CONFIG XENO_INPUT_PAD_CONFIRM
timeout 30s "$OUT/pad_confirm_test" write
timeout 30s "$OUT/pad_confirm_test" reload          # a new process: the restart
XENO_CONFIG="$OUT/custom/config.ini" bash -c 'mkdir -p "$(dirname "$XENO_CONFIG")"; timeout 30s "$0" preserve' "$OUT/pad_confirm_test"
# No XDG_CONFIG_HOME: ~/.config/xenogears-port/config.ini under a fresh HOME
export HOME="$OUT/home2"; mkdir -p "$HOME"; unset XDG_CONFIG_HOME
timeout 30s "$OUT/pad_confirm_test" write
timeout 30s "$OUT/pad_confirm_test" reload
echo "pad_confirm: all phases ok"
