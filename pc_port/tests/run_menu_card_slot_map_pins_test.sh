#!/usr/bin/env bash
# Pins MenuPsxPointerSlot's 32 card-screen slot pointers and the MENU_4CC
# mapping (see menu_card_slot_map_pins_test.c); the old 8-slot bound must be
# rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-slot-map.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
SRC="${CARD_SLOT_MAP_SOURCE:-src/menu/main/misc.c}"
awk '/^static u32\* MenuPsxPointerSlot\(/ {body=1} body {print} body && /^}/ {body=0}' "$SRC" > "$OUT/slot_map.inc"
test "$(grep -c '^static u32\* MenuPsxPointerSlot' "$OUT/slot_map.inc")" -eq 1
grep -q '#define MENU_4CC(p, off) ((u8\*)((SystemMenu\*)(p))->unk4CC + ((off) - 0x4CC))' "$SRC"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 -w pc_port/tests/menu_card_slot_map_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
sed -i 's/retailOffset < 0x428/retailOffset < 0x3C8/' "$OUT/slot_map.inc"
grep -q 'retailOffset < 0x3C8' "$OUT/slot_map.inc"
"${CC:-gcc}" "${BASE[@]}" -O0 -w pc_port/tests/menu_card_slot_map_pins_test.c -o "$OUT/eight"
if timeout 30s "$OUT/eight" >/dev/null 2>&1; then echo "FAIL mutant eight-slot-map accepted"; exit 1; fi
echo "REJECTED eight-slot-map"
echo "menu_card_slot_map_pins: PASS"
