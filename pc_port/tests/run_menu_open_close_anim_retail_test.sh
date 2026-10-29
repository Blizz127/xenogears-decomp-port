#!/usr/bin/env bash
# func_801D29A8 (menu open/close animation), shared retail/port body, vs the
# retail MIPS from disc/menu.bin.  O0/O2/UBSan plus mutants.
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/menu_retail_build.sh
test "$(sha256sum disc/menu.bin | cut -d' ' -f1)" = 82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-open-close-anim.XXXXXX")
python3 pc_port/tests/lib/gen_struct_mirror.py SystemMenu 1E98 > "$OUT/system_menu_mirror.inc"
HX_DEFSYMS=(D_801EA528=0x801EA528)
run_mode() {
  local mode=$1 src=$2 exe=$3
  hx_build_misc "$mode" "$exe.misc.o" "$src"
  gcc "${HX_COMMON[@]}" $(hx_mode_flags "$mode") -I"$OUT" -Wall -Wextra -Werror -Wno-unused-function -c pc_port/tests/menu_open_close_anim_retail_test.c -o "$exe.test.o"
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
mutate D29A8-settle-y '        MENU_OPEN_ANIM(1)->curY = 0x3E;' '        MENU_OPEN_ANIM(1)->curY = 0x3F;'
mutate D29A8-close-window '            g_Menu->pManager->shouldRenderWindow[0] = 0;' '            g_Menu->pManager->shouldRenderWindow[1] = 0;'
echo "PASS menu open close anim (O0/O2/UBSan + mutants)"
