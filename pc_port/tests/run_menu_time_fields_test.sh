#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-time-fields.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
s=next(pathlib.Path('asm/menu').glob('*matchings/main/misc*/func_801C7F34.s')).read_text()
pins=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s)
assert pins
for a,h in pins:
    off=int(a,16)-0x801c5000
    assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail time field instruction pins')
PY
awk '/^void func_801C7F34\(/ {body=1} body {print} /^}/ {body=0}' "${MENU_TIME_SOURCE:-src/menu/main/misc.c}" > "$OUT/time.inc"
test -s "$OUT/time.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
if [[ ${LEGACY_TIME:-0} == 1 ]]; then BASE+=(-DLEGACY_TIME); fi
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_time_fields_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    echo "RUN $mode"
    timeout 60s "$OUT/$mode"
done
cp "$OUT/time.inc" "$OUT/good.inc"
for mutant in wrong-divisor signed-input raw-offset byte-store lost-field clamped-leading; do
    case "$mutant" in
        wrong-divisor) sed 's/21600000, 2160000/21600001, 2160000/' "$OUT/good.inc" > "$OUT/time.inc" ;;
        signed-input) sed 's/ticks \/ divisors\[i\]/(s32)ticks \/ (s32)divisors[i]/' "$OUT/good.inc" > "$OUT/time.inc" ;;
        raw-offset) sed 's/g_Menu->unk2EC + i \* 4/(u8*)g_Menu + 0x2EC + i * 4/' "$OUT/good.inc" > "$OUT/time.inc" ;;
        byte-store) sed 's/\&value, sizeof(value)/\&value, 1/' "$OUT/good.inc" > "$OUT/time.inc" ;;
        lost-field) sed 's/i < 7/i < 6/' "$OUT/good.inc" > "$OUT/time.inc" ;;
        clamped-leading) sed '/ticks %= divisors\[i\];/a\        value %= 10;' "$OUT/good.inc" > "$OUT/time.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/time.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_time_fields_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/time.inc"
