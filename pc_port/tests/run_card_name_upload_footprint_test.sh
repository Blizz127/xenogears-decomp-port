#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/card-name-footprint.XXXXXX")
echo "OUTPUT $OUT"
echo '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d  disc/menu.bin' | sha256sum -c -
awk '/^static u32\* MenuPsxPointerSlot\(.*\) \{/ || /^static u8\* MenuRawPointer\(.*\) \{/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/pointer.inc"
awk '/^void func_801E61B0\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/init.inc"
awk '/^void func_801E920C\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/geometry.inc"
awk '/^void func_801D02D8\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/draw.inc"
for part in pointer init geometry draw; do test -s "$OUT/$part.inc"; done
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/card_name_upload_footprint_test.c -o "$OUT/$mode"
    timeout 10s "$OUT/$mode"
done
cp "$OUT/draw.inc" "$OUT/good.inc"
for marker in 130F 1311 per-panel; do
    case "$marker" in
        130F) sed 's/data\[0x130F\] \* sizeof(POLY_FT4)/data[0x130F] * 24/' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        1311) sed 's/meta\[0x1311\] \* sizeof(POLY_FT4)/meta[0x1311] * 24/' "$OUT/good.inc" > "$OUT/draw.inc" ;;
        per-panel) sed 's/data\[0x130F\] \* sizeof(POLY_FT4)/meta[0x130F] * sizeof(POLY_FT4)/' "$OUT/good.inc" > "$OUT/draw.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/draw.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/card_name_upload_footprint_test.c -o "$OUT/bad-$marker"
    if timeout 10s "$OUT/bad-$marker" > "$OUT/bad-$marker.log" 2>&1; then
        echo "FAIL bad stride survived: $marker"; exit 1
    fi
    grep -q Assertion "$OUT/bad-$marker.log"
    echo "REJECTED packet selector mutation: $marker"
done
