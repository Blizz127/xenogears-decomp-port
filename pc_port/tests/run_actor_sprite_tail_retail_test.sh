#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/actor_sprite_tail_retail_test
mkdir -p "$OUT"
read -r hash _ < <(dd if=disc/field.bin bs=1 skip=$((0x80076118-0x8006faf0)) count=$((0x1e8)) status=none | sha256sum)
test "$hash" = 66b6038c32ce09208817040ebddbacc8ff85835886f5b1a116e903a277a9b342
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/actor_sprite_tail_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.adapter.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.adapter.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in double_mask depth fog exclusive reread; do
    case "$mutant" in
        double_mask) expression='s/data\[0x3D\] = 0xEF/data[0x3D] = 0xFF/' ;;
        depth) expression='s/((u32)depth << 2)/((u32)(depth - 2) << 2)/g' ;;
        fog) expression='s/SpriteSetColor(sprite, actor\[0xFC\], actor\[0xFD\], actor\[0xFE\])/func_80075B08(sprite, actor + 0xFC)/' ;;
        exclusive) expression='s/if (\*(u32\*)(actor + 0x134) \& 0x40)/else if (*(u32*)(actor + 0x134) \& 0x40)/' ;;
        reread) expression='s/if (\*(u32\*)(actor + 0x134) \& 0x40)/if (flags \& 0x40)/' ;;
    esac
    sed "/^static int FieldRenderActorSpriteTail(/,/^}/ { $expression; }" \
        src/field/main/misc2.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DACTOR_RENDER_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/actor_sprite_tail_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.adapter.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "ACTOR TAIL mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'ACTOR TAIL FAIL' "$OUT/$mutant.log"
done
echo 'ACTOR TAIL negative controls PASS double mask/depth/fog/independent bits/flag reread'
