#!/usr/bin/env bash
# Pins func_801DD5E8's 12-row tint loop (see menu_ability_tint_pins_test.c)
# and checks that a body without the row loop is rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-ability-tint.XXXXXX")
echo "OUTPUT $OUT"
SRC="${ABILITY_TINT_SOURCE:-src/menu/main/misc.c}"
awk '/^void func_801DD5E8\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/tint.inc"
test "$(grep -c '^void func_801DD5E8' "$OUT/tint.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_ability_tint_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/tint.inc" "$OUT/good.inc"
sed -i 's/for (i = 0; i < 12; i++) {/for (i = 0; i < 0; i++) {/' "$OUT/tint.inc"
! cmp -s "$OUT/good.inc" "$OUT/tint.inc"
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_ability_tint_pins_test.c -o "$OUT/noloop"
if timeout 30s "$OUT/noloop" >/dev/null 2>&1; then echo "FAIL mutant no-row-loop accepted"; exit 1; fi
echo "REJECTED no-row-loop"
