#!/usr/bin/env bash
# Pins func_801C6D90's LoadImage RECT and func_801E64E0's ClearImage RECT and
# colour against the shared (retail-matching) bodies in src/menu/main/misc.c,
# and checks that the old shifted port arguments are rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-vram-rects.XXXXXX")
echo "OUTPUT $OUT"
SRC="${VRAM_RECT_SOURCE:-src/menu/main/misc.c}"
awk '/^void func_801C6D90\(/ || /^void func_801E64E0\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/rects.inc"
test "$(grep -c '^void func_801' "$OUT/rects.inc")" -eq 2
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_vram_rect_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/rects.inc" "$OUT/good.inc"
# Mutants: the pre-fix port arguments must fail.
for mutant in clut-at-origin clear-whole-screen clear-colour; do
    cp "$OUT/good.inc" "$OUT/rects.inc"
    case "$mutant" in
        clut-at-origin) sed -i 's/rect.y = 0x1C0;/rect.y = 0;/' "$OUT/rects.inc" ;;
        clear-whole-screen) sed -i 's/rect.x = 0x140;/rect.x = 0;/' "$OUT/rects.inc" ;;
        clear-colour) sed -i 's/ClearImage(&rect, 0, 0, 0);/ClearImage(\&rect, 0x40, 0x20, 0x20);/' "$OUT/rects.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/rects.inc"
    "${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_vram_rect_pins_test.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" >/dev/null 2>&1; then echo "FAIL mutant $mutant accepted"; exit 1; fi
    echo "REJECTED $mutant"
done
