#!/usr/bin/env bash
# Menu batch-init builders (func_801D7C3C callees) vs retail MIPS from disc/menu.bin.
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/menu_retail_build.sh
test "$(sha256sum disc/menu.bin | cut -d' ' -f1)" = 82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-batch-prims.XXXXXX")
python3 pc_port/tests/lib/gen_struct_mirror.py SystemMenu 1E98 > "$OUT/system_menu_mirror.inc"
HX_DEFSYMS=(g_GameState=0x8006D634 g_SystemPalette1=0x800595D4 g_SystemPalette2=0x80059414
  D_801EA578=0x801EA578 D_801EA5C4=0x801EA5C4 D_801E9D38=0x801E9D38 D_801E9D3C=0x801E9D3C
  D_801E9CF0=0x801E9CF0 D_801E9CF4=0x801E9CF4 D_801E9CF8=0x801E9CF8 D_801E9CFC=0x801E9CFC)
HX_DEFSYMS+=(D_801EA39C=0x801EA39C D_801E9B60=0x801E9B60 D_801E9C20=0x801E9C20)
for a in D0 D4 D8 DC E0 E4 E8 EC; do HX_DEFSYMS+=(D_801E9C$a=0x801E9C$a); done
for a in 00 04 08 0C 10 14 18 1C 20 24 28 2C 30 34; do HX_DEFSYMS+=(D_801E9D$a=0x801E9D$a); done
run_mode() {
  local mode=$1 src=$2 exe=$3
  hx_build_misc "$mode" "$exe.misc.o" "$src"
  gcc "${HX_COMMON[@]}" $(hx_mode_flags "$mode") -I"$OUT" -Wall -Wextra -Werror -Wno-unused-function -c pc_port/tests/menu_batch_prims_retail_test.c -o "$exe.test.o"
  gcc "${HX_COMMON[@]}" $(hx_mode_flags "$mode") -c pc_port/src/battle_mips_adapter.c -o "$exe.cpu.o"
  hx_link "$mode" "$exe" "$exe.test.o" "$exe.misc.o" "$exe.cpu.o"
}
SRC=${MENU_SRC:-src/menu/main/misc.c}
for mode in O0 O2 UBSan; do
  run_mode "$mode" "$SRC" "$OUT/$mode"
  "$OUT/$mode" disc/menu.bin
done
mutate() { # name, python replace pair
  python3 - "$SRC" "$OUT/$1.c" "$2" "$3" <<'PY'
import sys, pathlib
s = pathlib.Path(sys.argv[1]).read_text(); a, b = sys.argv[3], sys.argv[4]
assert s.count(a) >= 1, a
pathlib.Path(sys.argv[2]).write_text(s.replace(a, b, 1))
PY
  run_mode O2 "$OUT/$1.c" "$OUT/$1"
  if "$OUT/$1" disc/menu.bin > "$OUT/$1.log" 2>&1 || ! grep -q '^HX FAIL' "$OUT/$1.log"; then
    echo "MUTANT SURVIVED: $1"; exit 1; fi
  echo "MUTANT REJECTED: $1 ($(grep -m1 '^HX FAIL' "$OUT/$1.log" | cut -c1-90))"
}
mutate 5ED4-parity 'parity = g_Menu->pManager->currentCharacterIDs[slot] & 1;' 'parity = g_Menu->pManager->currentCharacterIDs[slot] & 2;'
mutate 5ED4-width 'width = (u16)(variant * 0x18 + 0x48);' 'width = (u16)(variant * 0x10 + 0x48);'
mutate 5ED4-shift '(u16)(D_801E9D38 - shift)' '(u16)(D_801E9D38 + shift)'
mutate 6338-cursor '    for (i = 0; i < count; i++, x += 8) {' '    for (i = 0; i < count; i++) {'
mutate 6338-gear-max 'x = D_801E9CF8 - 0x20;' 'x = D_801E9CF8 - 0x18;'
mutate 6338-height '(u16)(*(u16*)(prim + 0x17E2) - *(u16*)(prim + 0x17CA))' '(u16)(*(u16*)(prim + 0x17E0) - *(u16*)(prim + 0x17CA))'
mutate 680C-maxcursor '        MenuBatchDigits(start, count, D_801E9D08 - shift, D_801E9D0C, 0x2AEA, 0x1AE0, 0);' '        MenuBatchDigits(start, count, D_801E9D08 - shift, D_801E9D0C, 0x2AEA, 0x1AE0, 1);'
mutate 680C-fuelhalf 'fuel)[1]' 'fuel)[0]'
mutate 6CF4-tint '            ((POLY_FT4*)prim)->g0 = 0x80;' '            ((POLY_FT4*)prim)->g0 = 0x7F;'
mutate 7154-field 'field_0x0[0x40]' 'field_0x0[0x44]'
mutate 74EC-start 'MenuBatchDigits(2, 7, D_801E9D28' 'MenuBatchDigits(1, 7, D_801E9D28'
mutate 7884-tenths 'tenths = (s32)(value - whole * 100) / 10;' 'tenths = (s32)(value - whole * 100) % 10;'
mutate 7884-scale 'raw[0x79]) * 22)' 'raw[0x79]) * 21)'
mutate 6194-prims 'buf + buf[0x2AEC] * 0x50 + 0xA0,' 'buf + buf[0x2AEC] * 0x28 + 0xA0,'
mutate 7C3C-rc 'MENU_RAW(0x358)[0x2AE0] = *(u8*)&g_Menu->renderContext;' 'MENU_RAW(0x358)[0x2AE0] = (u8)g_Menu->unk2D8;'
echo "PASS menu batch prims (O0/O2/UBSan + mutants)"
