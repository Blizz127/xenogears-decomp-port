#!/usr/bin/env bash
# func_801E53CC (window frame-primitive init), shared retail/port body, vs the
# retail MIPS from disc/menu.bin.  O0/O2/UBSan plus mutants.
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/menu_retail_build.sh
test "$(sha256sum disc/menu.bin | cut -d' ' -f1)" = 82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-window-frame-init.XXXXXX")
python3 pc_port/tests/lib/gen_struct_mirror.py SystemMenu 1E98 > "$OUT/system_menu_mirror.inc"
HX_DEFSYMS=()
run_mode() {
  local mode=$1 src=$2 exe=$3
  hx_build_misc "$mode" "$exe.misc.o" "$src"
  gcc "${HX_COMMON[@]}" $(hx_mode_flags "$mode") -I"$OUT" -Wall -Wextra -Werror -Wno-unused-function -c pc_port/tests/menu_window_frame_init_retail_test.c -o "$exe.test.o"
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
mutate E53CC-bg-g2 'setRGB2(&pWindow->polysBackground[i], 0x68, 0x68, 0x68);' 'setRGB2(&pWindow->polysBackground[i], 0x68, 0x67, 0x68);'
mutate E53CC-left-clut 'GetClut(g_Menu->clutX2, g_Menu->clutY2)' 'GetClut(g_Menu->clutX1, g_Menu->clutY2)'
echo "PASS menu window frame init (O0/O2/UBSan + mutants)"
