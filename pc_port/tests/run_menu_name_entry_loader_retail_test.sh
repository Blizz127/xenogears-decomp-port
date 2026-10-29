#!/usr/bin/env bash
# Name entry resource loader NameEntryMenu_801C5318 (NON_MATCHING port body)
# vs retail MIPS from disc/name_entry_menu.bin (archive dir 0x10 file 8), with
# stubbed file I/O; O0 / O2 / UBSan, then deliberate mutants must be rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/menu_retail_build.sh
test "$(sha256sum disc/name_entry_menu.bin | cut -d' ' -f1)" = b0c6226511fdf0d3b874cf5e8658ed1fe19fd06833c0e0d61c4762aeb6a6f1b0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-name-entry-loader.XXXXXX")
python3 pc_port/tests/lib/gen_struct_mirror.py SystemMenu 1E98 > "$OUT/system_menu_mirror.inc"
python3 pc_port/tests/lib/gen_struct_mirror.py MenuUnk2 5034 > "$OUT/menu_unk2_mirror.inc"
# The TU's bzero/memmove are retail jal targets: route them to recording
# stubs (memcpy is inlined in retail and stays libc).  No fortify wrappers.
HX_COMMON+=(-U_FORTIFY_SOURCE)
HX_DEFSYMS=(g_GameState=0x8006D634 D_80059171=0x80059171 g_MenuDebugEnabled=0x80059178
  NameEntryMenu_D_801C5000=0x801C5000)
run_mode() {
  local mode=$1 src=$2 exe=$3
  ( HX_COMMON+=(-Dmemmove=NeHxMemmove -Dbzero=NeHxBzero); hx_build_misc "$mode" "$exe.misc.o" "$src" )
  gcc "${HX_COMMON[@]}" $(hx_mode_flags "$mode") -I"$OUT" -Wall -Wextra -Werror -Wno-unused-function -c pc_port/tests/menu_name_entry_loader_retail_test.c -o "$exe.test.o"
  gcc "${HX_COMMON[@]}" $(hx_mode_flags "$mode") -c pc_port/src/battle_mips_adapter.c -o "$exe.cpu.o"
  hx_link "$mode" "$exe" "$exe.test.o" "$exe.misc.o" "$exe.cpu.o"
}
SRC=src/name_entry_menu/main/misc.c
for mode in O0 O2 UBSan; do
  run_mode "$mode" "$SRC" "$OUT/$mode"
  "$OUT/$mode" disc/name_entry_menu.bin
done
mutate() { # name, unique text in the 801C5318 port body, replacement
  python3 - "$SRC" "$OUT/$1.c" "$2" "$3" <<'PY'
import sys, pathlib
s = pathlib.Path(sys.argv[1]).read_text(); a, b = sys.argv[3], sys.argv[4]
lo = s.index('void NameEntryMenu_801C5318(void) {'); hi = s.index('\n}\n', lo)
assert s.count(a) == 1 and lo < s.index(a) < hi, a
pathlib.Path(sys.argv[2]).write_text(s.replace(a, b, 1))
PY
  run_mode O2 "$OUT/$1.c" "$OUT/$1"
  if "$OUT/$1" disc/name_entry_menu.bin > "$OUT/$1.log" 2>&1 || ! grep -q '^HX FAIL' "$OUT/$1.log"; then
    echo "MUTANT SURVIVED: $1"; exit 1; fi
  echo "MUTANT REJECTED: $1 ($(grep -m1 '^HX FAIL' "$OUT/$1.log" | cut -c1-90))"
}
mutate gear20-id 'if (D_80059171 - 0xB == 9) {' 'if (D_80059171 - 0xB == 8) {'
mutate gear20-flag 'g_GameState.unk1D30 & 0x400' 'g_GameState.unk1D30 & 0x200'
mutate kind 'g_Menu->unk1E20->unk0[0xDE9] = 9;' 'g_Menu->unk1E20->unk0[0xDE9] = 8;'
mutate id-bytes 'NAME_TABLE[D_80059171 * 3 + 0x21];' 'NAME_TABLE[D_80059171 * 3 + 0x22];'
mutate pilot 'NAME_TABLE[D_80059171 - 0xB];' 'NAME_TABLE[D_80059171 - 0xA];'
mutate pos-adjust 'pos.word[1][4] = (u32)pos.word[1][4] + 12;' 'pos.word[1][4] = (u32)pos.word[1][4] + 10;'
mutate face-stride 'characterID * 0xB20' 'characterID * 0xB00'
mutate prect-y 'tim.prect->y = *(u16*)((u8*)&pos + 20 + i * 24);' 'tim.prect->y = *(u16*)((u8*)&pos + 16 + i * 24);'
mutate header-byte 'g_Menu->unk32C->unk4B95 = 0x43;' 'g_Menu->unk32C->unk4B95 = 0x44;'
mutate header-id 'NameEntryMenu_D_801C5000, 13);' 'NameEntryMenu_D_801C5000, 12);'
mutate seds-read 'ArchiveReadFileToBuffer(5, D_8006259C, 0, 0x80);' 'ArchiveReadFileToBuffer(5, D_8006259C, 0, 0x40);'
mutate seds-store 'g_Menu->unk2E4 = D_8006259C;' 'g_Menu->unk2E4 = 0;'
echo "PASS name entry loader (O0/O2/UBSan + mutants)"
