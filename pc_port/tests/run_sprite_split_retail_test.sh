#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_split_retail_test
mkdir -p "$OUT"
read -r hash _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x800+0xee88)) count=$((0x34c)) status=none | sha256sum)
test "$hash" = 0eb777659e58db935469d9c2414d85e6fc6f8434e2d80d1c86f9e54f13445c0c
read -r hash _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x800+0xf1d4)) count=$((0x35c)) status=none | sha256sum)
test "$hash" = cc1fa7d97c9c15195a062e2be867d115201f0b97597ca94de0993dae3fcf7103
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/sprite_split_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.adapter.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.adapter.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in capacity reservation equality mirror tag; do
    case "$mutant" in
        capacity) expression='s/cursor + count \* 40 >= end/cursor + count * 40 > end/' ;;
        reservation) expression='s/g_GfxCurWorkBuffer = poly + 40/g_GfxCurWorkBuffer = poly/' ;;
        equality) expression='s/high < split/high <= split/' ;;
        mirror) expression='s/frameFlags \& 16/frameFlags \& 32/g' ;;
        tag) expression='s/(\*(u32\*)ot \& 0xFF000000u)/0u/' ;;
    esac
    sed "/^static void RenderSpriteVerticalSplit(/,/^}/ { $expression; }" \
        src/slus_006.64/system/rendering.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DSPRITE_RENDER_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/sprite_split_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.adapter.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "SPRITE SPLIT mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'SPRITE SPLIT FAIL' "$OUT/$mutant.log"
done
echo 'SPRITE SPLIT negative controls PASS capacity/reservation/equality/mirror/tag'
