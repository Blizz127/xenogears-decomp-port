#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
overlay=disc/battle.bin
echo "1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291  $overlay" | sha256sum --check --status
out=$(mktemp -d "${TMPDIR:-/tmp}/xeno-hud-values.XXXXXX")
trap 'rm -rf "$out"' EXIT
cc -std=gnu17 -O0 -DXENO_PC_PORT -Ipc_port/src \
    pc_port/tests/battle_72f38_values_test.c pc_port/src/battle_mips_adapter.c \
    -lcrypto -o "$out/test"
XENO_DISABLE_CPU_INTERPRETER=0 timeout 60 "$out/test" "$overlay"
