#!/usr/bin/env bash
# Pins func_801CD710's state values (see menu_card_state_pins_test.c) and
# checks that the old mode-0 value 1 is rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-state.XXXXXX")
echo "OUTPUT $OUT"
SRC="${CARD_STATE_SOURCE:-src/menu/main/misc.c}"
awk '/^s32 func_801CD710\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/state.inc"
test "$(grep -c '^s32 func_801CD710' "$OUT/state.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_card_state_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/state.inc" "$OUT/good.inc"
sed -i 's/(pData + 0x334) = 7;/(pData + 0x334) = 1;/' "$OUT/state.inc"
! cmp -s "$OUT/good.inc" "$OUT/state.inc"
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_card_state_pins_test.c -o "$OUT/mode0-is-1"
if timeout 30s "$OUT/mode0-is-1" >/dev/null 2>&1; then echo "FAIL mutant mode0-is-1 accepted"; exit 1; fi
echo "REJECTED mode0-is-1"
