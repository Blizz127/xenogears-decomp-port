#!/usr/bin/env bash
# Pins func_801E20C8 (see menu_status_loop_pins_test.c) and checks that the
# old signed type test and the old two-argument step are rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-status-loop.XXXXXX")
echo "OUTPUT $OUT"
SRC="${STATUS_LOOP_SOURCE:-src/menu/main/misc.c}"
awk '/^s32 func_801E20C8\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/status.inc"
test "$(grep -c '^s32 func_801E20C8' "$OUT/status.inc")" -eq 1
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_status_loop_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/status.inc" "$OUT/good.inc"
mutant() {
    local name=$1 expr=$2
    cp "$OUT/good.inc" "$OUT/status.inc"
    sed -i "$expr" "$OUT/status.inc"
    ! cmp -s "$OUT/good.inc" "$OUT/status.inc"
    "${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_status_loop_pins_test.c -o "$OUT/$name"
    if timeout 30s "$OUT/$name" >/dev/null 2>&1; then echo "FAIL mutant $name accepted"; exit 1; fi
    echo "REJECTED $name"
}
mutant signed-type '0,/if ((u32)(g_Menu->pManager->currentCharacterIDs\[slotIdx\] - 7) < 2)/s//if ((s32)(g_Menu->pManager->currentCharacterIDs[slotIdx] - 7) < 2)/'
mutant no-current 's/func_801D9704(slotIdx, 0, 0)/func_801D9704(0, 0, 0)/'
