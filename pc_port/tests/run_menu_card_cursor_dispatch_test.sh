#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-cursor-dispatch.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import hashlib,pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
for name in ['801CA750','801CA480','801CA5F0','801C9EF4','801CA1D4']:
    kind='nonmatchings' if name in ['801C9EF4','801CA1D4'] else 'matchings'
    s=next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    pins=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s)
    assert pins,name
    for a,h in pins:
        off=int(a,16)-0x801c5000
        assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail dispatcher and four navigation instruction pins')
PY
awk '/^s32 func_801CA750\(s32 direction, s32 unused, s32 limit\) \{/ {body=1} body {print} /^}/ {body=0}' "${CARD_DISPATCH_SOURCE:-src/menu/main/misc.c}" > "$OUT/dispatch.inc"
test -s "$OUT/dispatch.inc"
for routine in 801C9EF4 801CA1D4 801CA480 801CA5F0; do
    awk -v fn="$routine" '$0 ~ "^void func_" fn "\\(" {body=1} body {print} /^}/ {body=0}' "${CARD_DISPATCH_SOURCE:-src/menu/main/misc.c}" > "$OUT/$routine.inc"
    test -s "$OUT/$routine.inc"
done
cat "$OUT/801C9EF4.inc" "$OUT/801CA1D4.inc" "$OUT/801CA480.inc" "$OUT/801CA5F0.inc" > "$OUT/navigation.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for integration in boundary integrated; do
extra=(); if [[ "$integration" == integrated ]]; then extra=(-DINTEGRATED_NAVIGATION); fi
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${extra[@]}" "${flags[@]}" pc_port/tests/menu_card_cursor_dispatch_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    echo "RUN $integration $mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/dispatch.inc" "$OUT/good.inc"
for mutant in wrong-route lost-result wrong-selection stale-refresh extra-byte; do
    case "$mutant" in
        wrong-route) sed 's/func_801C9EF4(direction/func_801CA1D4(direction/' "$OUT/good.inc" > "$OUT/dispatch.inc" ;;
        lost-result) sed 's/result = 2;/result = 0;/' "$OUT/good.inc" > "$OUT/dispatch.inc" ;;
        wrong-selection) sed 's/func_801C9EF4(direction, \*(s32\*)((u8\*)g_Menu->unk32C + MENU_CARD_OFF(0x4F7C)))/func_801C9EF4(direction, 0)/' "$OUT/good.inc" > "$OUT/dispatch.inc" ;;
        stale-refresh) sed '/func_801E781C(entry/{n;s/data = (u8\*)g_Menu->unk32C;/;/}' "$OUT/good.inc" > "$OUT/dispatch.inc" ;;
        extra-byte) sed '/= \*(s32\*)(data + MENU_CARD_OFF(0x4F7C));/a\        data[MENU_CARD_OFF(0x4F84)] = 0;' "$OUT/good.inc" > "$OUT/dispatch.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/dispatch.inc"
    "${CC:-gcc}" "${BASE[@]}" "${extra[@]}" -O2 pc_port/tests/menu_card_cursor_dispatch_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $integration $mutant"
done
cp "$OUT/good.inc" "$OUT/dispatch.inc"
done
cp "$OUT/navigation.inc" "$OUT/navigation-good.inc"
for routine in 801C9EF4 801CA1D4 801CA480 801CA5F0; do
    for mutant in ignored-class lost-store; do
        case "$mutant" in
            ignored-class) sed 's/mode == 0 ||/1 ||/g; s/direction == 0 ||/1 ||/g' "$OUT/$routine.inc" > "$OUT/mutated.inc" ;;
            lost-store) sed 's/card->unk4F7C = next;/(void)next;/g' "$OUT/$routine.inc" > "$OUT/mutated.inc" ;;
        esac
        ! cmp -s "$OUT/$routine.inc" "$OUT/mutated.inc"
        for part in 801C9EF4 801CA1D4 801CA480 801CA5F0; do
            if [[ "$part" == "$routine" ]]; then cat "$OUT/mutated.inc"; else cat "$OUT/$part.inc"; fi
        done > "$OUT/navigation.inc"
        "${CC:-gcc}" "${BASE[@]}" -DINTEGRATED_NAVIGATION -O2 pc_port/tests/menu_card_cursor_dispatch_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$routine-$mutant"
        if timeout 10s "$OUT/$routine-$mutant" > "$OUT/$routine-$mutant.log" 2>&1; then echo "FAIL mutant survived: $routine $mutant"; exit 1; fi
        grep -q Assertion "$OUT/$routine-$mutant.log"
        echo "REJECTED integrated $routine $mutant"
    done
done
cp "$OUT/navigation-good.inc" "$OUT/navigation.inc"
