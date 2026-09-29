#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-card-events.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
for name in ['801C8960','801D9B08','801C881C','801C87C4','801C9270']:
    s=next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    for a,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s):
        off=int(a,16)-0x801c5000
        assert b[off:off+4]==bytes.fromhex(h),a
print('PASS retail card event instruction pins')
PY
"${CC:-gcc}" -std=gnu17 -O2 -Ipc_port/src pc_port/tests/menu_card_classifier_boundary_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/classifier-boundary"
"$OUT/classifier-boundary"
awk '/^(static u32 MenuCardEvent\(|static void MenuSetCardEvent\(|s32 func_801C881C\(|void func_801C87C4\(|void func_801C8960\(|void func_801D9B08\()/ {body=1} body {print} /^}/ {body=0}' "${CARD_EVENTS_SOURCE:-src/menu/main/misc.c}" > "$OUT/events.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_card_events_test.c -o "$OUT/$mode"
    "$OUT/$mode"
done
cp "$OUT/events.inc" "$OUT/good.inc"
for mutant in wide-stride extra-byte truthy-event wrong-spec; do
    case "$mutant" in
        wide-stride) sed 's/slot \* 4/slot * 8/g' "$OUT/good.inc" > "$OUT/events.inc" ;;
        extra-byte) sed '/p\[3\] = (u8)(event >> 24);/a\    p[4] = 0;' "$OUT/good.inc" > "$OUT/events.inc" ;;
        truthy-event) sed 's/)) == 1/)) != 0/g' "$OUT/good.inc" > "$OUT/events.inc" ;;
        wrong-spec) sed 's/0x8000/0x100/g' "$OUT/good.inc" > "$OUT/events.inc" ;;
    esac
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_card_events_test.c -o "$OUT/$mutant"
    if "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q 'Assertion' "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/events.inc"
