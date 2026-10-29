#!/usr/bin/env bash
# Oversized battle data bindings (pc_port/tests/check_battle_binding_sizes.py):
# fails when a build adds a binding wider than its host object that
# battle_binding_sizes.allow does not list, or changes/fixes a listed one.
# usage: run_battle_binding_sizes_test.sh [pc_port/build_native/xeno-port]
set -euo pipefail
cd "$(dirname "$0")/../.."
BIN=${1:-pc_port/build_native/xeno-port}
exec python3 pc_port/tests/check_battle_binding_sizes.py "$BIN" \
    "$(dirname "$BIN")/battle_bridge_map.inc" pc_port/src/battle_mips_runtime.c \
    --allow pc_port/tests/battle_binding_sizes.allow
