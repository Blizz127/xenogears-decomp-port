#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/card-title-raster.XXXXXX")
echo "OUTPUT $OUT"
echo '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d  disc/menu.bin' | sha256sum -c -
awk '/^void func_801E6544\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/compress.inc"
awk '/^#define func_801E6668 / {print} /^void func_801E6668\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/raster.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
EXTRA=()
if [[ "${CARD_TITLE_REAL_KROM:-0}" == 1 ]]; then
    echo '11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef  disc/scph5500.bin' | sha256sum -c -
    export XENO_BIOS=disc/scph5500.bin
    awk '/^void\* func_801E65E4\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/convert.inc"
    BASE+=(-DREAL_KROM)
    EXTRA=(pc_port/src/krom_mapping.c pc_port/src/krom_rom.c $(pkg-config --cflags --libs libcrypto) -pthread)
elif [[ "${CARD_TITLE_NO_BIOS:-0}" == 1 ]]; then
    awk '/^void\* func_801E65E4\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/convert.inc"
    BASE+=(-DNO_BIOS_KROM)
    EXTRA=(pc_port/src/krom_mapping.c pc_port/src/krom_rom.c $(pkg-config --cflags --libs libcrypto) -pthread)
fi
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/card_title_raster_test.c pc_port/src/battle_mips_adapter.c "${EXTRA[@]}" -o "$OUT/$mode"
    if [[ "${CARD_TITLE_NO_BIOS:-0}" == 1 ]]; then
        env -u XENO_BIOS timeout 180s "$OUT/$mode"
    else
        timeout 180s "$OUT/$mode"
    fi
done
cp "$OUT/raster.inc" "$OUT/good.inc"
for mutant in wrong-card-offset byte-order short-glyph wrong-row-stride wrong-index-stride wrong-limit no-clear; do
    case "$mutant" in
        wrong-card-offset) sed 's/unkB94 + 4/unkB94 + 0x100/' "$OUT/good.inc" > "$OUT/raster.inc" ;;
        byte-order) sed 's/glyph\[row \* 2\]/glyph[row * 2 + 1]/' "$OUT/good.inc" > "$OUT/raster.inc" ;;
        short-glyph) sed 's/row < 16/row < 15/g' "$OUT/good.inc" > "$OUT/raster.inc" ;;
        wrong-row-stride) sed 's/row \* 64/row * 60/' "$OUT/good.inc" > "$OUT/raster.inc" ;;
        wrong-index-stride) sed 's/(index % 16) \* 4/(index % 16) * 3/' "$OUT/good.inc" > "$OUT/raster.inc" ;;
        wrong-limit) sed 's/++index >= 32/++index >= 31/' "$OUT/good.inc" > "$OUT/raster.inc" ;;
        no-clear) sed '/bzero(image, 0x1000);/d' "$OUT/good.inc" > "$OUT/raster.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/raster.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/card_title_raster_test.c pc_port/src/battle_mips_adapter.c "${EXTRA[@]}" -o "$OUT/$mutant"
    if timeout 180s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
