#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-wait.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
for name in ['801C8CA4','801C8D1C']:
    s=next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    for a,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s):
        off=int(a,16)-0x801c5000
        assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail wait instruction pins')
PY
awk '/^void func_801C8(CA4|D1C)\(u8 idx\) \{/ {body=1} body {print} /^}/ {body=0}' "${CARD_WAIT_SOURCE:-src/menu/main/misc.c}" > "$OUT/waits.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_wait_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/waits.inc" "$OUT/good.inc"
for mutant in short-wait wrong-sentinel wrong-port narrow-status retained-state wrong-pump; do
    case "$mutant" in
        short-wait) sed 's/i = 0x3B/i = 0x3A/g' "$OUT/good.inc" > "$OUT/waits.inc" ;;
        wrong-sentinel) sed 's/val != -2/val != -1/g' "$OUT/good.inc" > "$OUT/waits.inc" ;;
        wrong-port) sed 's/idx \* 4/(1 - idx) * 4/g' "$OUT/good.inc" > "$OUT/waits.inc" ;;
        narrow-status) sed 's/val != -2/(s16)val != -2/g' "$OUT/good.inc" > "$OUT/waits.inc" ;;
        retained-state) sed 's/\[MENU_CARD_OFF(0x4FE6)\] = 0;/[MENU_CARD_OFF(0x4FE6)] = 2;/' "$OUT/good.inc" > "$OUT/waits.inc" ;;
        wrong-pump) sed 's/            func_801C7BF4();/            Vsync(0);/' "$OUT/good.inc" > "$OUT/waits.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/waits.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_wait_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 5s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/waits.inc"
