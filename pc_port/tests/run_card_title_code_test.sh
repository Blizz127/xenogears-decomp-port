#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/card-title-code.XXXXXX")
echo "OUTPUT $OUT"
echo '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d  disc/menu.bin' | sha256sum -c -
awk '/^void\* func_801E65E4\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/convert.inc"
test -s "$OUT/convert.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -I"$OUT" -Ipc_port/src -Iinclude)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/card_title_code_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/convert.inc" "$OUT/good.inc"
for mutant in control-lead swapped-table wide-flag wrong-threshold narrow-pointer; do
    case "$mutant" in
        narrow-pointer) sed 's/return (void\*)PcPortKromFont(code, 32);/return (void*)(intptr_t)Krom2RawAdd(code);/' "$OUT/good.inc" > "$OUT/convert.inc" ;;
        control-lead) sed 's/code = 0x8140/code = 0x0040/' "$OUT/good.inc" > "$OUT/convert.inc" ;;
        swapped-table) sed 's/code = D_801EA5D0\[pChar\[0\]\];/code = (D_801EA5D0[pChar[0]] >> 8) | (D_801EA5D0[pChar[0]] << 8);/' "$OUT/good.inc" > "$OUT/convert.inc" ;;
        wide-flag) sed 's/D_801EA8C0 = 1/D_801EA8C0 = 0/' "$OUT/good.inc" > "$OUT/convert.inc" ;;
        wrong-threshold) sed 's/pChar\[0\] < 0x80/pChar[0] < 0x7F/' "$OUT/good.inc" > "$OUT/convert.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/convert.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/card_title_code_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
