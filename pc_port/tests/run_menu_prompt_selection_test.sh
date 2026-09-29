#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-selection.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import pathlib,re
b=pathlib.Path('disc/menu.bin').read_bytes()
for kind,name in [('nonmatchings','801CAA38'),('matchings','801CACF8')]:
    s=next(pathlib.Path('asm/menu').glob(f'*matchings/main/misc*/func_{name}.s')).read_text()
    for a,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',s):
        off=int(a,16)-0x801c5000
        assert b[off:off+4]==bytes.fromhex(h), a
print('PASS retail selection instruction pins')
PY
awk '/^(u32 func_801CAA38\(u8|u8 func_801CACF8\()/ {body=1} body {print} /^}/ {body=0}' "${PROMPT_SELECTION_SOURCE:-src/menu/main/misc.c}" > "$OUT/selection.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/menu_prompt_selection_test.c -o "$OUT/$mode"
    "$OUT/$mode"
done
cp "$OUT/selection.inc" "$OUT/good.inc"
for mutant in lost-result wrong-second-prompt timeout-extra-frame missing-cancel missing-card-mode; do
    case "$mutant" in
        lost-result) sed 's/^    MENU_CALL_D32B4();/    result = MENU_CALL_D32B4();/' "$OUT/good.inc" > "$OUT/selection.inc" ;;
        wrong-second-prompt) sed 's/func_801D2F4C(arg1);/func_801D2F4C(arg0);/' "$OUT/good.inc" > "$OUT/selection.inc" ;;
        timeout-extra-frame) sed 's/--remaining == 0/remaining-- == 0/' "$OUT/good.inc" > "$OUT/selection.inc" ;;
        missing-cancel) sed '/D_801EA8FC = 1;/d' "$OUT/good.inc" > "$OUT/selection.inc" ;;
        missing-card-mode) sed '/unk4F80\[0x66\] = 2;/d' "$OUT/good.inc" > "$OUT/selection.inc" ;;
    esac
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/menu_prompt_selection_test.c -o "$OUT/$mutant"
    if "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant"; exit 1; fi
    grep -q 'Assertion' "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
cp "$OUT/good.inc" "$OUT/selection.inc"
