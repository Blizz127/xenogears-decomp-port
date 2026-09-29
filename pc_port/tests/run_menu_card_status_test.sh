#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-status.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
s=next(pathlib.Path('asm/menu').glob('*matchings/main/misc*/func_801C8A10.s')).read_text()
for a,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s):
    off=int(a,16)-0x801c5000
    assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail status instruction pins')
PY
awk '/^s32 func_801C8A10\(u8 port\) \{/ {body=1} body {print} /^}/ {body=0}' "${CARD_STATUS_SOURCE:-src/menu/main/misc.c}" > "$OUT/status.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_status_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/status.inc" "$OUT/good.inc"
for mutant in transition-store record-stride wrong-port wrong-return lost-change; do
    case "$mutant" in
        transition-store) sed 's/+ MENU_CARD_OFF(0x4F74)) = 0;/+ MENU_CARD_OFF(0x4F74)) = 1;/' "$OUT/good.inc" > "$OUT/status.inc" ;;
        record-stride) sed 's/(port \* 16 + i) \* 0x5C/(port * 16 + i) * 0x3C/' "$OUT/good.inc" > "$OUT/status.inc" ;;
        wrong-port) sed 's/((s32\*)\&D_801EA900)\[port\] = 0;/((s32*)\&D_801EA900)[0] = 0;/' "$OUT/good.inc" > "$OUT/status.inc" ;;
        wrong-return) sed 's/if (result == -2) {/if (result == -1) {/' "$OUT/good.inc" > "$OUT/status.inc" ;;
        lost-change) sed 's/+ port)\[MENU_CARD_OFF(0x4F88)\] = 0;/+ port)[MENU_CARD_OFF(0x4F88)] = 1;/' "$OUT/good.inc" > "$OUT/status.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/status.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_status_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 5s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/status.inc"
