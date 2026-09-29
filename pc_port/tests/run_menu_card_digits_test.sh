#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-digits.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
for name,kind in [('801E6B70','matchings'),('801C80B8','nonmatchings')]:
    s=next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    pins=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s)
    assert pins
    for a,h in pins:
        off=int(a,16)-0x801c5000
        assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail card digits and decimal converter instruction pins')
PY
awk '/^static u32\* MenuPsxPointerSlot\(.*\) \{/ || /^static u8\* MenuRawPointer\(.*\) \{/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/pointer.inc"
awk '/^void func_801C80B8\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/decimal.inc"
awk '/^void func_801E6B70\(/ {body=1} body {print} /^}/ {body=0}' "${CARD_DIGITS_SOURCE:-src/menu/main/misc.c}" > "$OUT/digits.inc"
test -s "$OUT/digits.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_digits_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/digits.inc" "$OUT/good.inc"
for mutant in wrong-stride missing-base wrong-glyph-stride wrong-x lost-result stale-owner wrong-digit lost-tail; do
    case "$mutant" in
        wrong-stride) sed 's/slotIdx \* 0x87C/slotIdx * 0x7C/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        missing-base) sed 's/+ 0xA98 + 0x320/+ 0xA98/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        wrong-glyph-stride) sed 's/count \* 0x50/count * 0x54/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        wrong-x) sed 's/D_801EA01C + slotIdx \* 0x50/D_801EA01C + slotIdx * 0x30/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        lost-result) sed 's/+= (u8)written/+= 1/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        stale-owner) sed '/^            panel = MenuRawPointer(0x34C);/d' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        wrong-digit) sed 's/digits\[6 + i\]/digits[5 + i]/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        lost-tail) sed '/^    func_801C80B8(pTable\[slotIdx + 0x19\]);/d' "$OUT/good.inc" > "$OUT/digits.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/digits.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_digits_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/digits.inc"
