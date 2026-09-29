#!/usr/bin/env bash
# Pins func_801D9C84's failure return (see menu_card_enter_pins_test.c) and
# checks that the old "0 only while the prompt is up" variant is rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-enter.XXXXXX")
echo "OUTPUT $OUT"
SRC="${CARD_ENTER_SOURCE:-src/menu/main/misc.c}"
awk '/^s32 func_801D9C84\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/enter.inc"
test "$(grep -c '^s32 func_801D9C84' "$OUT/enter.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 -w pc_port/tests/menu_card_enter_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/enter.inc" "$OUT/good.inc"
python3 - "$OUT/enter.inc" <<'PY'
import sys
p=sys.argv[1]; s=open(p).read()
a='''        result = 0;
        if (((u8*)g_Menu->pManager)[0x33] != 0) {'''
assert a in s
s=s.replace(a,'''        if (((u8*)g_Menu->pManager)[0x33] != 0) {
            result = 0;''')
open(p,'w').write(s)
PY
"${CC:-gcc}" "${BASE[@]}" -O0 -w pc_port/tests/menu_card_enter_pins_test.c -o "$OUT/nested"
if timeout 30s "$OUT/nested" >/dev/null 2>&1; then echo "FAIL mutant nested-result accepted"; exit 1; fi
echo "REJECTED nested-result"
cp "$OUT/good.inc" "$OUT/enter.inc"
sed -i 's/MENU_CARD_OFF(\(0x[0-9A-F]*\))/\1/g' "$OUT/enter.inc"
! cmp -s "$OUT/good.inc" "$OUT/enter.inc"
"${CC:-gcc}" "${BASE[@]}" -O0 -w pc_port/tests/menu_card_enter_pins_test.c -o "$OUT/rawoff"
if timeout 30s "$OUT/rawoff" >/dev/null 2>&1; then echo "FAIL mutant raw-offsets accepted"; exit 1; fi
echo "REJECTED raw-offsets"
