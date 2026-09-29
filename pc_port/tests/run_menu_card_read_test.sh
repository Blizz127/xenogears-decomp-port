#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-read.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
s=next(pathlib.Path('asm/menu').glob('*matchings/main/misc*/func_801C90B0.s')).read_text()
for a,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s):
    off=int(a,16)-0x801c5000
    assert b[off:off+4]==bytes.fromhex(h),a
assert b[0xa8:0xae]==b'bu00:\0' and b[0xb0:0xb6]==b'bu10:\0'
print('PASS retail card-read instruction and prefix pins')
PY
awk '/^void func_801C90B0\(/ {body=1} body {print} /^}/ {body=0}' "${CARD_READ_SOURCE:-src/menu/main/misc.c}" > "$OUT/card_read.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_read_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/card_read.inc" "$OUT/good.inc"
for mutant in record-stride block-stride failure-shortcut stale-owner wrong-port retained-index; do
    case "$mutant" in
        record-stride) sed 's/index \* 0x5C/index * 0x28/g' "$OUT/good.inc" > "$OUT/card_read.inc" ;;
        block-stride) sed 's/index \* 0x200/index * 0x100/g' "$OUT/good.inc" > "$OUT/card_read.inc" ;;
        failure-shortcut) sed 's/^    func_801C9038(pathBuf, \(.*\));/    if (func_801C9038(pathBuf, \1) == -1) return;/' "$OUT/good.inc" > "$OUT/card_read.inc" ;;
        stale-owner) sed '/^    func_801C9038(pathBuf,/i\    SystemMenu* stale = g_Menu;' "$OUT/good.inc" | sed '/^    func_801C9038(pathBuf,/a\    g_Menu = stale;' > "$OUT/card_read.inc" ;;
        wrong-port) sed 's/0x2E + port \* 16/0x2E/g' "$OUT/good.inc" > "$OUT/card_read.inc" ;;
        retained-index) sed 's/++writeIndex;/writeIndex += 0;/' "$OUT/good.inc" > "$OUT/card_read.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/card_read.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_read_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/card_read.inc"
