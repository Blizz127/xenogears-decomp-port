#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-halfword.XXXXXX")
echo "OUTPUT $OUT"
routine=${CARD_DIGIT_ROUTINE:-801E6CFC}
case "$routine" in
    801E6CFC) variant=-DWORD_DIGITS ;;
    801E6F5C) variant=-DBYTE_PAIR ;;
    *) echo "FAIL unsupported routine: $routine"; exit 1 ;;
esac
python3 - "$routine" <<'PY'
import hashlib,pathlib,re,sys
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
for name in [sys.argv[1],'801C80B8']:
    s=next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    pins=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s)
    assert pins
    for a,h in pins:
        off=int(a,16)-0x801c5000
        assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail',sys.argv[1],'and decimal converter instruction pins')
PY
awk '/^static u32\* MenuPsxPointerSlot\(.*\) \{/ || /^static u8\* MenuRawPointer\(.*\) \{/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/pointer.inc"
awk '/^void func_801C80B8\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/decimal.inc"
awk -v fn="$routine" '$0 ~ "^void func_" fn "\\(" {body=1} body {print} /^}/ {body=0}' "${CARD_HALFWORD_SOURCE:-src/menu/main/misc.c}" > "$OUT/digits.inc"
if [[ ! -s "$OUT/digits.inc" ]]; then echo "FAIL native $routine implementation missing"; exit 1; fi
BASE=(-std=gnu17 "$variant" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_digits_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    echo "RUN $mode"
    timeout 60s "$OUT/$mode"
done
cp "$OUT/digits.inc" "$OUT/good.inc"
for mutant in wrong-value wrong-counter wrong-base fixed-columns lost-result stale-owner wrong-stride; do
    case "$mutant" in
        wrong-value) sed 's/row \* 6/row * 4/; s/row \* 3/row * 2/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        wrong-counter) sed 's/0x130A + row/0x130A/; s/0x130C + row/0x130C/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        wrong-base) sed 's/? 0x500 : 0x5F0/? 0x510 : 0x5F0/; s/? 0x6E0 : 0x780/? 0x6F0 : 0x780/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        fixed-columns) sed 's/(row == 0 ? i : column)/i/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        lost-result) sed 's/+= (u8)written/+= 1/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        stale-owner) sed '/^                panel = MenuRawPointer(0x34C);/d' "$OUT/good.inc" > "$OUT/digits.inc" ;;
        wrong-stride) sed 's/slotIdx \* 0x87C/slotIdx * 0x7C/' "$OUT/good.inc" > "$OUT/digits.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/digits.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_digits_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/digits.inc"
