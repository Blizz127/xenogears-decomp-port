#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_shadow_retail_test
mkdir -p "$OUT"
read -r hash _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x800+0xe9bc)) count=$((0x4ac)) status=none | sha256sum)
test "$hash" = ae85540de0cc7aabc12334f1860d166b96827448b47aa6326d68d633f9c898be
read -r hash _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x800+0x3fad8)) count=32 status=none | sha256sum)
test "$hash" = 66687aadf862bd776c8fc18b8e9f8e20089714856ee233b3902a591d0d5f2925
read -r hash _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x800+0x3faf8)) count=16 status=none | sha256sum)
test "$hash" = 4c0ed3ab5045f9ec560b3d482485fb7c664fde704241a992ffeab246c6cbf426
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/sprite_shadow_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.adapter.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.adapter.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in half_scale height matrix mask average_round average_wrap color uv; do
    case "$mutant" in
        half_scale) expression='s/scale.vx \/ 2/(scale.vx >> 1)/' ;;
        height) expression='s/data + 0x84/data + 6/' ;;
        matrix) expression='s/SetTransMatrix(\&matrix)/(void)0/' ;;
        mask) expression='s/\& data\[0x3D\]) == 0/\& data[0x3D]) != 0/' ;;
        average_round) expression='s/average \/ 2/(average >> 1)/g' ;;
        average_wrap) expression='s/s16 average/s32 average/; s/average = (s16)(/average = (/' ;;
        color) expression='s/0x2C000000u/0x2C808080u/' ;;
        uv) expression='s/frame\[4\] + frame\[6\] - 1/frame[4] + frame[6]/' ;;
    esac
    sed "/^void func_8001E9BC(/,/^}/ { $expression; }" \
        src/slus_006.64/system/rendering.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DSPRITE_RENDER_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/sprite_shadow_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.adapter.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "SPRITE SHADOW mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'SPRITE SHADOW FAIL' "$OUT/$mutant.log"
done
echo 'SPRITE SHADOW negative controls PASS scale/height/matrix/mask/round/wrap/color/UV'
