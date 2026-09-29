#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-cursor-single.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
for name in ['801CA480','801CA5F0']:
    s=next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    for a,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s):
        off=int(a,16)-0x801c5000
        assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail single-position cursor instruction pins')
PY
BASE=(-std=gnu17 -DCURSOR_SINGLE -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for routine in ${SINGLE_ROUTINES:-801CA480 801CA5F0}; do
    extra=(); if [[ "$routine" == 801CA5F0 ]]; then extra=(-DCURSOR_BACKWARD); fi
    awk -v fn="$routine" '$0 ~ "^void func_" fn "\\(" {body=1} body {print} /^}/ {body=0}' "${CARD_SINGLE_SOURCE:-src/menu/main/misc.c}" > "$OUT/cursor_single.inc"
    test -s "$OUT/cursor_single.inc"
    for mode in O0 O2 UBSan; do
        flags=(-"$mode")
        if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
        "${CC:-gcc}" "${BASE[@]}" "${extra[@]}" "${flags[@]}" pc_port/tests/menu_card_cursor_down_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$routine-$mode"
        echo "RUN $routine $mode"
        timeout 30s "$OUT/$routine-$mode"
    done
    cp "$OUT/cursor_single.inc" "$OUT/good.inc"
    for mutant in used-limit ignored-class skipped-entry presence-column lost-store; do
        case "$mutant" in
            used-limit) sed '/(void)limit;/a\    if (limit == -1) return;' "$OUT/good.inc" > "$OUT/cursor_single.inc" ;;
            ignored-class) sed 's/direction == 0 ||/1 ||/g' "$OUT/good.inc" > "$OUT/cursor_single.inc" ;;
            skipped-entry) sed 's/(u32)next + 1/(u32)next + 2/g; s/(u32)next - 1/(u32)next - 2/g' "$OUT/good.inc" > "$OUT/cursor_single.inc" ;;
            presence-column) sed 's/next \/ 15/startIdx \/ 15/' "$OUT/good.inc" > "$OUT/cursor_single.inc" ;;
            lost-store) sed 's/card->unk4F7C = next;/(void)next;/g' "$OUT/good.inc" > "$OUT/cursor_single.inc" ;;
        esac
        ! cmp -s "$OUT/good.inc" "$OUT/cursor_single.inc"
        "${CC:-gcc}" "${BASE[@]}" "${extra[@]}" -O2 pc_port/tests/menu_card_cursor_down_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$routine-$mutant"
        if timeout 10s "$OUT/$routine-$mutant" > "$OUT/$routine-$mutant.log" 2>&1; then echo "FAIL mutant survived: $routine $mutant"; exit 1; fi
        grep -q Assertion "$OUT/$routine-$mutant.log"
        echo "REJECTED $routine $mutant"
    done
    cp "$OUT/good.inc" "$OUT/cursor_single.inc"
done
