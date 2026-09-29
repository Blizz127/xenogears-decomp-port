#!/usr/bin/env bash
# Pins func_801E5924 (see menu_title_brackets_pins_test.c) and checks that the
# old 2-byte table stride and the old x + 16 middle vertex are rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-title-brackets.XXXXXX")
echo "OUTPUT $OUT"
SRC="${TITLE_BRACKETS_SOURCE:-src/menu/main/misc.c}"
awk '/^void func_801E5924\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/brackets.inc"
test "$(grep -c '^void func_801E5924' "$OUT/brackets.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_title_brackets_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/brackets.inc" "$OUT/good.inc"
mutant() {
    local name=$1 expr=$2
    cp "$OUT/good.inc" "$OUT/brackets.inc"
    sed -i "$expr" "$OUT/brackets.inc"
    ! cmp -s "$OUT/good.inc" "$OUT/brackets.inc"
    "${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_title_brackets_pins_test.c -o "$OUT/$name"
    if timeout 30s "$OUT/$name" >/dev/null 2>&1; then echo "FAIL mutant $name accepted"; exit 1; fi
    echo "REJECTED $name"
}
mutant half-stride 's/&D_801E9894\[slotIdx \* 2\]/\&D_801E9894[slotIdx]/'
mutant mid-x16 's/((LINE_F3\*)((u8\*)p + 0x80))->x1 = \*pX;/((LINE_F3*)((u8*)p + 0x80))->x1 = *pX + 0x10;/'
