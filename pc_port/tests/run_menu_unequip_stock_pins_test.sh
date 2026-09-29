#!/usr/bin/env bash
# Pins func_801E0434 (see menu_unequip_stock_pins_test.c) and checks that the
# old insert-a-duplicate-below-the-cap variant is rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-unequip-stock.XXXXXX")
echo "OUTPUT $OUT"
SRC="${UNEQUIP_STOCK_SOURCE:-src/menu/main/misc.c}"
awk '/^void func_801E0434\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/stock.inc"
test "$(grep -c '^void func_801E0434' "$OUT/stock.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_unequip_stock_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/stock.inc" "$OUT/good.inc"
python3 - "$OUT/stock.inc" <<'PY'
import sys
p=sys.argv[1]; s=open(p).read()
a='''            if (++pCounter[i] >= 100) {
                pCounter[i] = 99;
            }
            found = 0;'''
assert a in s
s=s.replace(a,'''            if (++pCounter[i] >= 100) {
                pCounter[i] = 99;
                found = 0;
            }''')
open(p,'w').write(s)
PY
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_unequip_stock_pins_test.c -o "$OUT/dup"
if timeout 30s "$OUT/dup" >/dev/null 2>&1; then echo "FAIL mutant dup-insert accepted"; exit 1; fi
echo "REJECTED dup-insert"
