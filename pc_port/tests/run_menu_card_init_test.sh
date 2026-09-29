#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-init.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
s=next(pathlib.Path('asm/menu').glob('*matchings/main/misc*/func_801E61B0.s')).read_text()
pins=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s)
assert pins
for a,h in pins:
    off=int(a,16)-0x801c5000
    assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail card initializer instruction pins')
PY
awk '/^static u32\* MenuPsxPointerSlot\(.*\) \{/ || /^static u8\* MenuRawPointer\(.*\) \{/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/pointer.inc"
awk '/^void func_801E61B0\(/ {body=1} body {print} /^}/ {body=0}' "${CARD_INIT_SOURCE:-src/menu/main/misc.c}" > "$OUT/init.inc"
if [[ ! -s "$OUT/init.inc" ]]; then echo 'FAIL native 801E61B0 implementation missing'; exit 1; fi
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_init_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/init.inc" "$OUT/good.inc"
for mutant in wrong-sentinel lost-panel lost-result wrong-base wrong-table stale-page wrong-marker; do
    case "$mutant" in
        wrong-sentinel) sed 's/!= 0xFFFF/!= 0xFFFFFFFF/' "$OUT/good.inc" > "$OUT/init.inc" ;;
        lost-panel) sed 's/slot < 3/slot < 2/' "$OUT/good.inc" > "$OUT/init.inc" ;;
        lost-result) sed 's/+= (u8)written/+= 1/' "$OUT/good.inc" > "$OUT/init.inc" ;;
        wrong-base) sed 's/+ 0xA98 + 0x50/+ 0xA98 + 0x54/' "$OUT/good.inc" > "$OUT/init.inc" ;;
        wrong-table) sed 's/slot \* 4/slot * 2/g' "$OUT/good.inc" > "$OUT/init.inc" ;;
        stale-page) sed '/page = GetTPage/,+1{/^        p = /d;}' "$OUT/good.inc" > "$OUT/init.inc" ;;
        wrong-marker) sed 's/offset + 0x1311/offset + 0x1310/' "$OUT/good.inc" > "$OUT/init.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/init.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_init_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/init.inc"
