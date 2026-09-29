#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-cursor-retreat.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
s=next(pathlib.Path('asm/menu').glob('*matchings/main/misc*/func_801CA1D4.s')).read_text()
for a,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s):
    off=int(a,16)-0x801c5000
    assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail cursor retreat instruction pins')
PY
awk '/^void func_801CA1D4\(s32 mode, s32 selected\) \{/ {body=1} body {print} /^}/ {body=0}' "${CARD_RETREAT_SOURCE:-src/menu/main/misc.c}" > "$OUT/cursor_up.inc"
test -s "$OUT/cursor_up.inc"
BASE=(-std=gnu17 -DCURSOR_BACKWARD -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_cursor_down_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/cursor_up.inc" "$OUT/good.inc"
for mutant in zero-boundary table-offset ignored-class presence-column fallback-distance; do
    case "$mutant" in
        zero-boundary) sed 's/next >= 0/next > 0/g' "$OUT/good.inc" > "$OUT/cursor_up.inc" ;;
        table-offset) sed 's/D_801E981C\[next\]/D_801E981C[next + 1]/g' "$OUT/good.inc" > "$OUT/cursor_up.inc" ;;
        ignored-class) sed 's/mode == 0 ||/1 ||/g' "$OUT/good.inc" > "$OUT/cursor_up.inc" ;;
        presence-column) sed 's/next \/ 15/selected \/ 15/' "$OUT/good.inc" > "$OUT/cursor_up.inc" ;;
        fallback-distance) sed 's/unk4F7C - 4/unk4F7C - 3/g' "$OUT/good.inc" > "$OUT/cursor_up.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/cursor_up.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_cursor_down_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/cursor_up.inc"
