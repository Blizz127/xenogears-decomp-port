#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/model_prim_fae8_retail_test
routine=FAE8
if [ "${1:-}" = --f4b4 ]; then
    routine=F4B4
    OUT=pc_port/build_native/model_prim_f4b4_retail_test
elif [ "$#" != 0 ]; then
    echo 'usage: run_model_prim_fae8_retail_test.sh [--f4b4]' >&2
    exit 2
fi
mkdir -p "$OUT"
check_slice() {
    local address="$1" size="$2" expected="$3" actual
    read -r actual _ < <(dd if=disc/SLUS_006.64 bs=1 \
        skip=$((address - 0x8000f800)) count=$((size)) status=none | sha256sum)
    test "$actual" = "$expected"
}
if [ "$routine" = F4B4 ]; then
    check_slice 0x8002f4b4 0x200 b5c1c8ba79f06d23330897a15f53351bcc505469014e82e82d45d5efd09d246a
    check_slice 0x8004fec8 0x28 e14d03d8f5aa0b942fb31757dd4d2a4fb551501bc794014924fee8004fe6d024
else
    check_slice 0x8002fae8 0x214 0aad615182563b964aa23bbc2447cf969136f2083ed6b27fcaf3683c1d55bd5c
    check_slice 0x8004ffb8 0x28 e49c9192d55f141c5fe8783484d6ce3f5f07bb17a35ec31b64c82dea52e76e40
fi
check_slice 0x8002e1f4 0x38 b7d98d849c2248e0da88f393f8e57a1f9b42df3c4d568ab3c194b01b91f5e2de
COMMON=(-fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0
    -ffunction-sections -fdata-sections
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/build_native)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${COMMON[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" \
            -o "$OUT/$opt.$name.o"
    done
    for pair in "test:pc_port/tests/model_prim_fae8_retail_test.c" \
                "cpu:pc_port/src/battle_mips_adapter.c" \
                "body:src/slus_006.64/system/temp2.c" \
                "model_link:pc_port/src/model_prim_link.c" \
                "guest_link:pc_port/src/guest_prim_link.c" \
                "compat:pc_port/src/psyq_compat.c" \
                "leaf:pc_port/src/retail_leaf_adapters.c"; do
        name=${pair%%:*}; source=${pair#*:}
        extra=(-Wall -Wextra -Werror)
        if [ "$name" = test ] && [ "$routine" = F4B4 ]; then extra+=(-DTEST_F4B4); fi
        if [ "$name" = body ] || [ "$name" = compat ]; then extra=(-fpermissive -w); fi
        gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" "${extra[@]}" \
            -c "$source" -o "$OUT/$opt.$name.o"
    done
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -ldl -o "$OUT/$opt.test"
    echo "MODEL PRIM $routine full production TU $opt"
    timeout 30s "$OUT/$opt.test"
done
mutants=(terminal_rtpt lookahead_v0 fourth_vertex normal_v0)
if [ "$routine" = F4B4 ]; then mutants=(terminal_rtpt partial_xy packed_v1 normal_v0); fi
for mutant in "${mutants[@]}"; do
    case "$mutant" in
        terminal_rtpt) expression='s/gte_rtpt();/if (remaining != 0) gte_rtpt();/' ;;
        lookahead_v0) expression='s/(cmd << 3) \& 0xfff8u/(cmd \& 0xffffu) << 3/' ;;
        fourth_vertex) expression='s/gte_rtps();/gte_rtpt();/' ;;
        normal_v0) expression='s/MTC2(\*(u32\*)(normals - 8), 0);/MTC2(0, 0);/' ;;
        partial_xy) expression='s/\*(u32\*)(out + 0x14) = xy1;/;/' ;;
        packed_v1) expression='s/(cmd >> 13) \& 0xfff8u/(cmd >> 16) << 3/' ;;
    esac
    if [ "$routine" = F4B4 ] && [ "$mutant" = normal_v0 ]; then
        expression='s/MTC2(\*(u32\*)n0, 0);/MTC2(0, 0);/'
    fi
    # Mechanical negative-control copies only; production source stays intact.
    sed "/^s32 func_8002${routine}(/,/^#endif/ { $expression; }" \
        src/slus_006.64/system/temp2.c > "$OUT/$mutant.c"
    gcc "${COMMON[@]}" "${GAME[@]}" -O2 -fpermissive -w \
        -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.model_link.o" "$OUT/O2.guest_link.o" \
        "$OUT/O2.compat.o" "$OUT/O2.leaf.o" "$OUT/O2.LIBGTE.C.o" \
        "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$OUT/$mutant.test"
    if timeout 30s "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "MODEL PRIM $routine MUTANT SURVIVED: $mutant" >&2
        exit 1
    fi
    rg -q "MODEL PRIM $routine FAIL" "$OUT/$mutant.log"
done
echo "MODEL PRIM $routine MUTANTS PASS ${mutants[*]}"
