#!/usr/bin/env bash
# Pins func_801C9270's card-directory match (see
# menu_card_dir_match_pins_test.c) and checks that the old 0x28 record
# stride and the old one-byte compare are both rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-dir-match.XXXXXX")
echo "OUTPUT $OUT"
SRC="${CARD_DIR_MATCH_SOURCE:-src/menu/main/misc.c}"
awk '/^void func_801C9270\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/match.inc"
test "$(grep -c '^void func_801C9270' "$OUT/match.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_card_dir_match_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/match.inc" "$OUT/good.inc"
mutant() {
    local name=$1 expr=$2
    cp "$OUT/good.inc" "$OUT/match.inc"
    sed -i "$expr" "$OUT/match.inc"
    ! cmp -s "$OUT/good.inc" "$OUT/match.inc"
    "${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_card_dir_match_pins_test.c -o "$OUT/$name"
    if timeout 30s "$OUT/$name" >/dev/null 2>&1; then echo "FAIL mutant $name accepted"; exit 1; fi
    echo "REJECTED $name"
}
mutant stride-0x28 's/\* 0x5C;/* 0x28;/'
mutant fixed-byte 's/pData\[MENU_CARD_OFF(0x4FCE) + j\]/pData[port * 0x10 + i + MENU_CARD_OFF(0x4FCE)]/'
mutant raw-offsets 's/MENU_CARD_OFF(\(0x[0-9A-F]*\))/\1/g'
