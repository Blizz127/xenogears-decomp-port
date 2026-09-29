#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-upload.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
s=next(pathlib.Path('asm/menu').glob('*matchings/main/misc*/func_801E71B4.s')).read_text()
pins=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s)
assert pins
for a,h in pins:
    off=int(a,16)-0x801c5000
    assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail card name upload instruction pins')
PY
awk '/^void func_801E71B4\(/ {body=1} body {print} /^}/ {body=0}' "${CARD_UPLOAD_SOURCE:-src/menu/main/misc.c}" > "$OUT/upload.inc"
test -s "$OUT/upload.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_upload_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/upload.inc" "$OUT/good.inc"
for mutant in wrong-pair wrong-count early-end wrong-atlas wrong-size missing-sync; do
    case "$mutant" in
        wrong-pair) sed 's/character \* 20 + i\]/character * 20 + (i ^ 2)]/g' "$OUT/good.inc" > "$OUT/upload.inc" ;;
        wrong-count) sed 's/decoded, i \/ 2/decoded, i \/ 4/' "$OUT/good.inc" > "$OUT/upload.inc" ;;
        early-end) sed 's/bytes\[i\] == 0 \&\&/bytes[i] == 0 ||/' "$OUT/good.inc" > "$OUT/upload.inc" ;;
        wrong-atlas) sed 's/\[charIdx \* 2\]/[charIdx]/g' "$OUT/good.inc" > "$OUT/upload.inc" ;;
        wrong-size) sed 's/HeapAlloc(0x3F6, 0)/HeapAlloc(0x3F4, 0)/' "$OUT/good.inc" > "$OUT/upload.inc" ;;
        missing-sync) sed '/^    DrawSync(0);/d' "$OUT/good.inc" > "$OUT/upload.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/upload.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_upload_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/upload.inc"
