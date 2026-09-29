#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp -d /tmp/xeno-battle-mips-test.XXXXXX)"
trap 'rm -rf "$OUT"' EXIT

gcc -std=gnu17 -Wall -Wextra -Werror \
    -I"$ROOT/pc_port/src" \
    "$ROOT/pc_port/tests/battle_mips_adapter_test.c" \
    "$ROOT/pc_port/src/battle_mips_adapter.c" \
    -o "$OUT/battle_mips_adapter_test"

"$OUT/battle_mips_adapter_test"
