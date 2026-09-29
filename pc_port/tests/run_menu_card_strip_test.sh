#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-strip.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
s=next(pathlib.Path('asm/menu').glob('*matchings/main/misc*/func_801E733C.s')).read_text()
pins=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s)
assert pins
for a,h in pins:
    off=int(a,16)-0x801c5000
    assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail card strip instruction pins')
PY
awk '/^static u32\* MenuPsxPointerSlot\(.*\) \{/ || /^static u8\* MenuRawPointer\(.*\) \{/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/pointer.inc"
awk '/^void func_801E733C\(/ {body=1} body {print} /^}/ {body=0}' "${CARD_STRIP_SOURCE:-src/menu/main/misc.c}" > "$OUT/strip.inc"
if [[ ! -s "$OUT/strip.inc" ]]; then echo 'FAIL native 801E733C implementation missing'; exit 1; fi
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_strip_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/strip.inc" "$OUT/good.inc"
for mutant in lost-quad wrong-width wrong-u wrong-v wrong-context stale-page stale-clut; do
    case "$mutant" in
        lost-quad) sed 's/i < 16/i < 15/' "$OUT/good.inc" > "$OUT/strip.inc" ;;
        wrong-width) sed 's/(x + 12)/(x + 13)/' "$OUT/good.inc" > "$OUT/strip.inc" ;;
        wrong-u) sed 's/i \* 16 + 12/i * 16 + 13/' "$OUT/good.inc" > "$OUT/strip.inc" ;;
        wrong-v) sed 's/= 0xFF;/= 0xFE;/' "$OUT/good.inc" > "$OUT/strip.inc" ;;
        wrong-context) sed 's/+ g_Menu->renderContext/+ 0/g' "$OUT/good.inc" > "$OUT/strip.inc" ;;
        stale-page) sed '/Retail reloads/,+2d' "$OUT/good.inc" > "$OUT/strip.inc" ;;
        stale-clut) sed '/clut = GetClut/,+2{/^        p = /d; /^            i \* 2/d;}' "$OUT/good.inc" > "$OUT/strip.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/strip.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_strip_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/strip.inc"
