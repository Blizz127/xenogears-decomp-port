#!/usr/bin/env bash
# Pins func_801CB8AC's wait/return semantics (see menu_card_wait_pins_test.c)
# and checks that the old return-1-on-change and 0x80-frame-cap variants are
# rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-wait.XXXXXX")
echo "OUTPUT $OUT"
SRC="${CARD_WAIT_SOURCE:-src/menu/main/misc.c}"
awk '/^u8 func_801CB8AC\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/wait.inc"
test "$(grep -c '^u8 func_801CB8AC' "$OUT/wait.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_card_wait_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/wait.inc" "$OUT/good.inc"
mutant() {
    local name=$1 expr=$2
    cp "$OUT/good.inc" "$OUT/wait.inc"
    sed -i "$expr" "$OUT/wait.inc"
    ! cmp -s "$OUT/good.inc" "$OUT/wait.inc"
    "${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_card_wait_pins_test.c -o "$OUT/$name"
    if timeout 30s "$OUT/$name" >/dev/null 2>&1; then echo "FAIL mutant $name accepted"; exit 1; fi
    echo "REJECTED $name"
}
mutant change-returns-1 's/                changed = 1;/                changed = 1; result = 1;/'
mutant frame-cap 's/} while (g_Menu->input == 8);/} while (g_Menu->input == 8 \&\& frames < 0x80);/'
mutant raw-offsets 's/MENU_CARD_OFF(\(0x[0-9A-F]*\))/\1/g'
