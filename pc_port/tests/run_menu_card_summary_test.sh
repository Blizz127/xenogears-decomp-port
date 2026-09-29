#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-summary.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
for name,kind in [('801E68AC','nonmatchings'),('801C7F34','matchings')]:
    s=next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    pins=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s)
    assert pins
    for a,h in pins:
        off=int(a,16)-0x801c5000
        assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail card summary and time formatter instruction pins')
PY
awk '/^static u32\* MenuPsxPointerSlot\(.*\) \{/ || /^static u8\* MenuRawPointer\(.*\) \{/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/pointer.inc"
awk '/^void func_801C7F34\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/time.inc"
awk '/^void func_801E68AC\(/ {body=1} body {print} /^}/ {body=0}' "${CARD_SUMMARY_SOURCE:-src/menu/main/misc.c}" > "$OUT/summary.inc"
if [[ ! -s "$OUT/summary.inc" ]]; then echo 'FAIL native 801E68AC implementation missing'; exit 1; fi
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_summary_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/summary.inc" "$OUT/good.inc"
for mutant in wrong-label wrong-table wrong-stride wrong-field cached-counter byte-wrap cached-owner; do
    case "$mutant" in
        wrong-label) sed 's/, 0xEE,/, 0xEF,/' "$OUT/good.inc" > "$OUT/summary.inc" ;;
        wrong-table) sed 's/D_801E9FE0\[2 + i\]/D_801E9FE0[1 + i]/' "$OUT/good.inc" > "$OUT/summary.inc" ;;
        wrong-stride) sed 's/i \* 0x50/i * 0x54/' "$OUT/good.inc" > "$OUT/summary.inc" ;;
        wrong-field) sed 's/unk2EC + i \* 4/unk2EC + i/' "$OUT/good.inc" > "$OUT/summary.inc" ;;
        cached-counter) sed '/entry\[0x23\].*\/ 10/i\    u32 savedCounter = entry[0x23];' "$OUT/good.inc" | sed '/% 10/s/(u32)entry\[0x23\]/savedCounter/' > "$OUT/summary.inc" ;;
        byte-wrap) sed 's/((u32)entry\[0x23\] + 1)/((u8)(entry[0x23] + 1))/g' "$OUT/good.inc" > "$OUT/summary.inc" ;;
        cached-owner) sed '/^    u32 ticks;/a\    SystemMenu* savedMenu = g_Menu;' "$OUT/good.inc" | sed 's/g_Menu->/savedMenu->/g' > "$OUT/summary.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/summary.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_summary_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/summary.inc"
