#!/usr/bin/env bash
# Pins func_801C8324's divide-by-256 done test (see menu_open_anim_pins_test.c)
# and checks that the old >> 8 form is rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-open-anim.XXXXXX")
echo "OUTPUT $OUT"
SRC="${OPEN_ANIM_SOURCE:-src/menu/main/misc.c}"
awk '/^typedef struct \{$/ {buf=$0 "\n"; t=1; next} t {buf=buf $0 "\n"; if ($0 ~ /^\} MenuOpenAnim;/) {printf "%s", buf; t=0}; if ($0 ~ /^\}/) t=0; next}
     /^#define MENU_OPEN_ANIM\(/ {print}
     /^void func_801C8324\(/ {body=1} body {print} body && /^}/ {body=0}' "$SRC" > "$OUT/anim.inc"
test "$(grep -c '^void func_801C8324' "$OUT/anim.inc")" -eq 1
test "$(grep -c '^} MenuOpenAnim;' "$OUT/anim.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_open_anim_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/anim.inc" "$OUT/good.inc"
sed -i 's|s->accX / 256|(s->accX >> 8)|g; s|s->accY / 256|(s->accY >> 8)|g' "$OUT/anim.inc"
! cmp -s "$OUT/good.inc" "$OUT/anim.inc"
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_open_anim_pins_test.c -o "$OUT/shift"
if timeout 30s "$OUT/shift" >/dev/null 2>&1; then echo "FAIL mutant shift accepted"; exit 1; fi
echo "REJECTED shift"
