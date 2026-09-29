#!/usr/bin/env bash
# Pins the 0xA4 g_GameState party-record stride of func_801E4258 and
# func_801E42AC (shared retail-matching bodies in src/menu/main/misc.c) and
# checks that the old 0x28 stride is rejected.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-party-records.XXXXXX")
echo "OUTPUT $OUT"
SRC="${PARTY_RECORD_SOURCE:-src/menu/main/misc.c}"
awk '/^void func_801E4258\(/ || /^void func_801E42AC\(/ || /^void func_801E41C0\(/ || /^void func_801E4998\(/ || /^void func_801E35BC\(/ {body=1} body {print} /^}/ {body=0}' "$SRC" > "$OUT/records.inc"
test "$(grep -c '^void func_801' "$OUT/records.inc")" -eq 5
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
"${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_party_record_pins_test.c -o "$OUT/good"
timeout 30s "$OUT/good"
cp "$OUT/records.inc" "$OUT/good.inc"
for mutant in stride-0x28 conditional-3f conditional-9f extra-halving slot-by-slotidx; do
    cp "$OUT/good.inc" "$OUT/records.inc"
    case "$mutant" in
        stride-0x28) sed -i '0,/idx \* 0xA4/s//idx * 0x28/' "$OUT/records.inc" ;;
        conditional-3f) sed -i 's/    pEntry\[0x3F\] = pTable\[0xE\];/    if (0) pEntry[0x3F] = pTable[0xE];/' "$OUT/records.inc" ;;
        conditional-9f) sed -i 's/    pEntry\[0x9F\] = pTable\[0x17\];/    if (0) pEntry[0x9F] = pTable[0x17];/' "$OUT/records.inc" ;;
        extra-halving) sed -i 's|(val / 10) \* 2 / 9;|(val / 10) * 2 / 9 / 2;|' "$OUT/records.inc" ;;
        slot-by-slotidx) sed -i 's/(uintptr_t)pCtx + charIdx \* 4 + 0x20/(uintptr_t)pCtx + slotIdx * 4 + 0x20/' "$OUT/records.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/records.inc"
    "${CC:-gcc}" "${BASE[@]}" -O0 pc_port/tests/menu_party_record_pins_test.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" >/dev/null 2>&1; then echo "FAIL mutant $mutant accepted"; exit 1; fi
    echo "REJECTED $mutant"
done
