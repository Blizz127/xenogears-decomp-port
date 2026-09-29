#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/card-icon-draw.XXXXXX")
echo "OUTPUT $OUT"
echo '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d  disc/menu.bin' | sha256sum -c -
awk '/^static u32\* MenuPsxPointerSlot\(.*\) \{/ || /^static u8\* MenuRawPointer\(.*\) \{/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/pointer.inc"
awk '/^void func_801D02D8\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/draw.inc"
test -s "$OUT/draw.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
if [[ "${CARD_DRAW_DETAIL:-0}" == 1 ]]; then BASE+=(-DCARD_DRAW_DETAIL); fi
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/card_icon_draw_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/draw.inc" "$OUT/good.inc"
mutants=(raw-selection raw-frame wrong-map stale-initial stale-clut stale-tail)
if [[ "${CARD_DRAW_DETAIL:-0}" == 1 ]]; then
    mutants=(detail-character detail-list detail-first detail-name detail-summary detail-suffix detail-strip wrong-count stale-tail)
fi
for mutant in "${mutants[@]}"; do
    case "$mutant" in
        detail-character) sed '/detail: next character owner/d' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        detail-list) sed '/detail: list owner/d' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        detail-first) sed '/detail: first quad owner/d' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        detail-name) sed '/detail: name quad owner/d' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        detail-summary) sed '/detail: summary owner/d' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        detail-suffix) sed '/detail: suffix owner/d' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        detail-strip) sed '/detail: strip owner/d' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        wrong-count) sed 's/0x1312,0x1308,0x1309/0x1312,0x1308,0x1308/' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        raw-selection) sed 's/manager->unk4F7C/(*(s32*)((u8*)manager + 0x4F7C))/' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        raw-frame) sed 's/memcpy(\&animation, g_Menu->unk4CC/memcpy(\&animation, (u8*)g_Menu + 0x4CC/' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        wrong-map) sed 's/selected\] + 0x2E/selected] + 0x2C/' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        stale-initial) sed '/u8 source = manager->unk0/{n;/data = MenuRawPointer/d;}' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        stale-clut) sed '/^            poly = data + g_Menu->renderContext \* 40;/d' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        stale-tail) sed '/data = MenuRawPointer.*Retail shared tail reload/d' "$OUT/good.inc" > "$OUT/draw.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/draw.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/card_icon_draw_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
