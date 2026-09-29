#!/usr/bin/env bash
# Pins func_801C9BCC (see menu_card_can_act_pins_test.c) and checks that the
# old mode-1 nesting and the old pManager destination are rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-can-act.XXXXXX")
echo "OUTPUT $OUT"
SRC="${CARD_CAN_ACT_SOURCE:-src/menu/main/misc.c}"
awk '/^s32 func_801C9BCC\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/act.inc"
test "$(grep -c '^s32 func_801C9BCC' "$OUT/act.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_card_can_act_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/act.inc" "$OUT/good.inc"
mutant() {
    local name=$1 expr=$2
    cp "$OUT/good.inc" "$OUT/act.inc"
    sed -i "$expr" "$OUT/act.inc"
    ! cmp -s "$OUT/good.inc" "$OUT/act.inc"
    "${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_card_can_act_pins_test.c -o "$OUT/$name"
    if timeout 30s "$OUT/$name" >/dev/null 2>&1; then echo "FAIL mutant $name accepted"; exit 1; fi
    echo "REJECTED $name"
}
mutant manager-dest 's/g_Menu->unk4CC\[0xC\] = 2;/((u8*)g_Menu->pManager)[0x4D8 % 0x600] = 2;/'
mutant mode1-no-used-check '0,/if ((pData + offset)\[MENU_CARD_OFF(0x4FAE)\] == 0xFF) goto fail;/s//;/'
mutant raw-offsets 's/MENU_CARD_OFF(\(0x[0-9A-F]*\))/\1/g'
