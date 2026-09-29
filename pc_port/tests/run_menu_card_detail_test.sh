#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-detail.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
s=next(pathlib.Path('asm/menu').glob('*matchings/main/misc*/func_801E76EC.s')).read_text()
pins=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s)
assert pins
for a,h in pins:
    off=int(a,16)-0x801c5000
    assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail card detail instruction pins')
PY
awk '/^static u32\* MenuPsxPointerSlot\(.*\) \{/ || /^static u8\* MenuRawPointer\(.*\) \{/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/pointer.inc"
awk '/^void func_801E76EC\(/ {body=1} body {print} /^}/ {body=0}' "${CARD_DETAIL_SOURCE:-src/menu/main/misc.c}" > "$OUT/detail.inc"
test -s "$OUT/detail.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_detail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/detail.inc" "$OUT/good.inc"
for mutant in wrong-entry return-pointer late-entry wrong-stride raw-context skipped-call empty-enabled; do
    case "$mutant" in
        wrong-entry) sed 's/unkB94 + 0x100/unkB94 + 0x101/' "$OUT/good.inc" > "$OUT/detail.inc" ;;
        return-pointer) sed 's/^    func_801E61B0();/    pEntry = func_801E61B0();/' "$OUT/good.inc" > "$OUT/detail.inc" ;;
        late-entry) sed '/^    func_801E61B0();/a\    pEntry = g_Menu->unk32C->unkB94 + 0x100 + (u32)screenIdx * 512u;' "$OUT/good.inc" > "$OUT/detail.inc" ;;
        wrong-stride) sed 's/slotOff += 0x87C/slotOff += 0x7C/' "$OUT/good.inc" > "$OUT/detail.inc" ;;
        raw-context) sed 's/(u8)g_Menu->renderContext/*(u8*)((u8*)g_Menu + 0x308)/' "$OUT/good.inc" > "$OUT/detail.inc" ;;
        skipped-call) sed '/^            func_801E6B70(idx, pEntry);/d' "$OUT/good.inc" > "$OUT/detail.inc" ;;
        empty-enabled) sed 's/slotOff + 0x1310) = 0/slotOff + 0x1310) = 1/' "$OUT/good.inc" > "$OUT/detail.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/detail.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_detail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/detail.inc"
