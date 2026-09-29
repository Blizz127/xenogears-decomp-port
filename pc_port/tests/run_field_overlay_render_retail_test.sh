#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_overlay_render_retail_test
mkdir -p "$OUT"
read -r hash _ < <(dd if=disc/field.bin bs=1 skip=$((0x800a84c0-0x8006faf0)) count=$((0x6e4)) status=none | sha256sum)
test "$hash" = 2b9ff04c20456340d56f8fa7df6c553297fbe6f568892fafad3b6c804cd2f724
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/field_overlay_render_retail_test.c -o "$OUT/$opt.test.o"
    for source in src/field/main/misc5.c src/field/main/misc4.c pc_port/src/data_field.c pc_port/src/battle_mips_adapter.c; do
        name=$(basename "$source")
        gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c "$source" -o "$OUT/$opt.$name.o"
    done
    objcopy --weaken-symbol=func_800A8314 "$OUT/$opt.misc5.c.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in count blink angle ot_tag counter; do
    case "$mutant" in
        count) expression='s/i < 109/i < 108/' ;;
        blink) expression='s/\& 0x10/\& 0x20/' ;;
        angle) expression='s/pitch >>= 2/pitch >>= 4/' ;;
        ot_tag) expression='s/(\*ot \& 0xff000000u)/0u/' ;;
        counter) expression='s/(u32)D_800AEB60 + 1u/(u32)D_800AEB60 + 2u/' ;;
    esac
    sed "/^void func_800A84C0(/,/^}/ { $expression; }" src/field/main/misc5.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    objcopy --weaken-symbol=func_800A8314 "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" \
        "$OUT/O2.misc4.c.o" "$OUT/O2.data_field.c.o" "$OUT/O2.battle_mips_adapter.c.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "OVERLAY RENDER mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'OVERLAY RENDER FAIL' "$OUT/$mutant.log"
done
echo 'OVERLAY RENDER negative controls PASS count/blink/angle/OT-tag/counter'
