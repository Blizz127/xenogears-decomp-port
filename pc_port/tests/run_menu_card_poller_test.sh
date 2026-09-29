#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-poller.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
for name in ['801C8BEC','801C8A10']:
    s=next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    for a,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s):
        off=int(a,16)-0x801c5000
        assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail poller instruction pins')
PY
awk '/^void func_801C8BEC\(void\) \{/ {body=1} body {print} /^}/ {body=0}' "${CARD_POLLER_SOURCE:-src/menu/main/misc.c}" > "$OUT/poller.inc"
awk '/^s32 func_801C8A10\(u8 port\) \{/ {body=1} body {print} /^}/ {body=0}' "${CARD_STATUS_SOURCE:-src/menu/main/misc.c}" > "$OUT/status.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_poller_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" -DINTEGRATED_STATUS pc_port/tests/menu_card_poller_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/integrated-$mode"
    timeout 30s "$OUT/integrated-$mode"
done
cp "$OUT/poller.inc" "$OUT/good.inc"
for mutant in inclusive-threshold wrong-port retained-counter wrong-status stale-owner; do
    case "$mutant" in
        inclusive-threshold) sed 's/counter > D_801E9779/counter >= D_801E9779/' "$OUT/good.inc" > "$OUT/poller.inc" ;;
        wrong-port) sed 's/func_801C8A10(1)/func_801C8A10(0)/' "$OUT/good.inc" > "$OUT/poller.inc" ;;
        retained-counter) sed 's/g_Menu->unk326 = 0;/g_Menu->unk326 = 1;/' "$OUT/good.inc" > "$OUT/poller.inc" ;;
        wrong-status) sed 's/a == -1 \&\&/a == -2 \&\&/' "$OUT/good.inc" > "$OUT/poller.inc" ;;
        stale-owner) sed 's/SystemMenu\* pMenu2 = g_Menu;/SystemMenu* pMenu2 = pMenu;/' "$OUT/good.inc" > "$OUT/poller.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/poller.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_poller_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
    "${CC:-gcc}" "${BASE[@]}" -O2 -DINTEGRATED_STATUS pc_port/tests/menu_card_poller_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/integrated-$mutant"
    if timeout 10s "$OUT/integrated-$mutant" > "$OUT/integrated-$mutant.log" 2>&1; then echo "FAIL integrated mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/integrated-$mutant.log"
    echo "REJECTED integrated-$mutant"
done
cp "$OUT/good.inc" "$OUT/poller.inc"
cp "$OUT/status.inc" "$OUT/good-status.inc"
for mutant in transition-store removal-stride; do
    case "$mutant" in
        transition-store) sed 's/+ MENU_CARD_OFF(0x4F74)) = 0;/+ MENU_CARD_OFF(0x4F74)) = 1;/' "$OUT/good-status.inc" > "$OUT/status.inc" ;;
        removal-stride) sed 's/(port \* 16 + i) \* 0x5C/(port * 16 + i) * 0x3C/' "$OUT/good-status.inc" > "$OUT/status.inc" ;;
    esac
    ! cmp -s "$OUT/good-status.inc" "$OUT/status.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 -DINTEGRATED_STATUS pc_port/tests/menu_card_poller_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL integrated status mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED integrated-$mutant"
done
cp "$OUT/good-status.inc" "$OUT/status.inc"
