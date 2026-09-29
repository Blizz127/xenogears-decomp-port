#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/battle_gte_retail_test
mkdir -p "$OUT"
read -r normal_long_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x80048d7c-0x8000f800)) count=$((0x2c)) status=none | sha256sum)
test "$normal_long_sha" = 110dd369388dc66e9dc7a19b713c1a23b4644224cedf24e837a3b88fe9db9149
read -r normal_core_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x80048dd8-0x8000f800)) count=$((0xbc)) status=none | sha256sum)
test "$normal_core_sha" = 7e6f8fdab6570a20c556da4d6e50ae83ff8934ea0592ba60da26d5a76e4e4db6
read -r square_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x8004a414 - 0x8000f800)) \
    count=$((0x28)) status=none | sha256sum)
test "$square_sha" = 99cb1c8d0c79529e8c6f1a56f640ebd7400c2eb0f57312cdc2e252c1e34f201b
read -r magnitude_sha _ < <(dd if=disc/field.bin bs=1 skip=$((0x80099a4c-0x8006faf0)) count=$((0x40)) status=none | sha256sum)
test "$magnitude_sha" = 63e8d69bc64a73a15ad3495fe8f13d4a792e16110ee37a8e4466a7105182674b
read -r sqrt_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x80048c4c-0x8000f800)) count=$((0x84)) status=none | sha256sum)
test "$sqrt_sha" = fda79f78e9d9d817a0a8e1612f3a55e14e3783c220b4d6e26bff8fdaf32ed391
read -r sqrt_table_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x80056a00-0x8000f800)) count=$((0x180)) status=none | sha256sum)
test "$sqrt_table_sha" = 45ca1b7b619b33961c03c82243b8baf59fc50c49b4d7eefc94b76edbd417a5c8
read -r sqrt_prefix_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x80056880-0x8000f800)) count=2 status=none | sha256sum)
test "$sqrt_prefix_sha" = eff6e7a1bc97fe7fd326c73642feefb306607803ff7d45a15beda347e3bc9c86
read -r normal_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x80048da8 - 0x8000f800)) \
    count=$((0xec)) status=none | sha256sum)
test "$normal_sha" = 2d59f42c6e5c2bf222028ae7d4c3abc908940a2ba5361e838a5acde0babc1df4
read -r normal_table_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x80056b14 - 0x8000f800)) \
    count=$((0x200)) status=none | sha256sum)
test "$normal_table_sha" = 7e7e21906ffb76fd4478baab9b5c10dafc148d2a7c99d22d92939262bb8db417
read -r slice_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x8004a64c - 0x8000f800)) \
    count=$((0x8004a8ec - 0x8004a64c)) status=none | sha256sum)
if [ "$slice_sha" != 489349145f714efe93b292802e106bb31da567da46aa54413098f3c4a528a2c2 ]; then
    echo "BATTLE GTE RETAIL SLICE MISMATCH: $slice_sha" >&2
    exit 1
fi
read -r matrix_sha _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x8004987c - 0x8000f800)) \
    count=$((0x110)) status=none | sha256sum)
if [ "$matrix_sha" != a009644e1a602c9bf37cc61fba50d68c4d375f183ff6d8547055db15258e6b5c ]; then
    echo "BATTLE GTE MATRIX RETAIL SLICE MISMATCH: $matrix_sha" >&2
    exit 1
fi
COMMON=(-fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${COMMON[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.$name.o"
    done
    gcc -std=gnu17 "${COMMON[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/build_native \
        -Wall -Wextra -Werror -c pc_port/tests/battle_gte_retail_test.c -o "$OUT/$opt.test.o"
    gcc -std=gnu17 "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    gcc -std=gnu17 "${COMMON[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        -Wall -Wextra -Werror -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$opt.leaves.o"
    gcc -std=gnu17 "${COMMON[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        -fpermissive -w -c pc_port/src/psyq_compat.c -o "$OUT/$opt.compat.o"
    gcc -std=gnu17 "${COMMON[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        -fpermissive -w -c src/field/main/misc7.c -o "$OUT/$opt.magnitude.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.leaves.o" "$OUT/$opt.magnitude.o" "$OUT/$opt.compat.o" "$OUT/$opt.LIBGTE.C.o" \
        "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -ldl -o "$OUT/$opt"
    "$OUT/$opt"
done
for mutant in square_scale square_store; do
    case "$mutant" in
        square_scale) expression='s/gte_sqr0()/gte_sqr12()/' ;;
        square_store) expression='s/gte_stlvnl(v1);/v1->vx = v1->vy = v1->vz = 0;/' ;;
    esac
    sed "/^VECTOR\* Square0(/,/^}/ { $expression; }" pc_port/src/psyq_compat.c > "$OUT/$mutant.c"
    gcc -std=gnu17 "${COMMON[@]}" -O2 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" \
        "$OUT/O2.leaves.o" "$OUT/O2.magnitude.o" "$OUT/$mutant.o" "$OUT/O2.LIBGTE.C.o" \
        "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$OUT/$mutant"
    if "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "SQUARE0 mutant survived: $mutant" >&2; exit 1
    fi
    rg -q '(SQUARE0|MAGNITUDE2) RETAIL FAIL' "$OUT/$mutant.log"
done
echo 'SQUARE0 negative controls PASS scale/result-store'
sed '/^int SquareRoot0(/,/^}/s/MTC2((unsigned int)a, 30);/(void)a;/' \
    pc_port/extern/PsyCross/src/psx/LIBGTE.C > "$OUT/sqrt-register.C"
g++ -std=c++17 "${COMMON[@]}" -O2 -fpermissive -w \
    -Ipc_port/extern/PsyCross/src/psx -include pc_port/src/port_compat.h \
    -c "$OUT/sqrt-register.C" -o "$OUT/sqrt-register.o"
clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" \
    "$OUT/O2.leaves.o" "$OUT/O2.magnitude.o" "$OUT/O2.compat.o" "$OUT/sqrt-register.o" \
    "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$OUT/sqrt-register"
if "$OUT/sqrt-register" > "$OUT/sqrt-register.log" 2>&1; then
    echo 'MAGNITUDE2 sqrt-register mutant survived' >&2; exit 1
fi
rg -q 'MAGNITUDE2 RETAIL FAIL' "$OUT/sqrt-register.log"
echo 'MAGNITUDE2 negative control PASS missing LZCS write'
for mutant in normal_lzcs normal_zero normal_gpf normal_store; do
    case "$mutant" in
        normal_lzcs) expression='s/MTC2(squared, 30);/MTC2(1, 30);/' ;;
        normal_zero) expression='s/0x1c6c/0x1000/' ;;
        normal_gpf) expression='s/0x0190003d/0x0198003d/' ;;
        normal_store) expression='s/v1->vz = z;/v1->vz = y;/' ;;
    esac
    sed "/^long VectorNormal(/,/^}/ { $expression; }" pc_port/src/psyq_compat.c > "$OUT/$mutant.c"
    gcc -std=gnu17 "${COMMON[@]}" -O2 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" \
        "$OUT/O2.leaves.o" "$OUT/O2.magnitude.o" "$OUT/$mutant.o" "$OUT/O2.LIBGTE.C.o" \
        "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$OUT/$mutant"
    if "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "NORMAL LONG mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'normal long mismatch' "$OUT/$mutant.log"
done
echo 'NORMAL LONG negative controls PASS LZCS/zero-table/GPF/store'
