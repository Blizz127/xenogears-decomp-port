#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/battle_animation_leaf_test
mkdir -p "$OUT"
check_slice() {
    local address="$1" size="$2" expected="$3" actual
    read -r actual _ < <(dd if=disc/battle.bin bs=1 \
        skip=$((address - 0x8006faf0)) count=$((size)) status=none | sha256sum)
    test "$actual" = "$expected"
}
check_slice 0x800b168c 0x18 e0dedf5e7431d44f04bd206c92e077bba44d06d0fc1d4d759b0317db5d0f53ef
check_slice 0x800b16a4 0x4c 407016beda83459888ae2a063ef1e73348d74f61f2b811a5917a74b70ace64c2
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    gcc -std=gnu17 "${common[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_animation_leaf_test.c -o "$OUT/$opt.test.o"
    gcc -std=gnu17 "${common[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    gcc -std=gnu17 "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -Iinclude \
        -Ipc_port/include_shim -c src/battle/main.c -o "$OUT/$opt.body.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in stride record_size record_step; do
    case "$mutant" in
        stride) expression='s/index \* 28u/index * 24u/' ;;
        record_size) expression='s/total += ((u32)record\[0\]/total += ((u32)record[1]/' ;;
        record_step) expression='s/record += ((u32)record\[1\]/record += ((u32)record[0]/' ;;
    esac
    sed "$expression" src/battle/main.c > "$OUT/$mutant.c"
    gcc -std=gnu17 -O2 -fno-pie -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" \
        "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "BATTLE ANIM LEAF mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'BATTLE ANIM LEAF FAIL' "$OUT/$mutant.log"
done
echo 'BATTLE ANIM LEAF negative controls PASS stride/record-size/record-step'
