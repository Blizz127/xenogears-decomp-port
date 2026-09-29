#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=pc_port/build_native/field_particle_render_ot_retail_test
mkdir -p "$OUT"
read -r retail_sha _ < <(dd if=disc/field.bin bs=1 \
    skip=$((0x800a9b54 - 0x8006faf0)) count=$((0x3c4)) status=none | sha256sum)
if [ "$retail_sha" != d1272f8027d3f58fc692d437bdcd7e701f58d23fbfeeeeb41f46ae7016c1df38 ]; then
    echo 'PARTICLE RENDER OT RETAIL SLICE MISMATCH' >&2
    exit 1
fi
sed -n '/^void FieldMatrixCopyTransform(/,/^}/p' src/field/main/misc2.c > "$OUT/matrix_copy.c"
test "$(rg -c '^void FieldMatrixCopyTransform\(' "$OUT/matrix_copy.c")" = 1
COMMON=(-fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${COMMON[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.$name.o"
    done
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -fpermissive -w \
        -c src/field/effects/particles.c -o "$OUT/$opt.native.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -include common.h -include field/main.h -c "$OUT/matrix_copy.c" -o "$OUT/$opt.copy.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/field_particle_render_ot_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.native.o" \
        "$OUT/$opt.copy.o" "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.LIBGTE.C.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" \
        -o "$OUT/$opt.test"
    timeout 45s "$OUT/$opt.test"
done

# Mutate generated test copies, never production: each half of the old mixed
# layout must fail even when the other half is correct.
test "$(rg -F -c '+ 0xCC);' src/field/effects/particles.c)" = 1
test "$(rg -F -c 'addPrim(ot + value,' src/field/effects/particles.c)" = 1
for mutant in offset stride; do
    if [ "$mutant" = offset ]; then
        sed 's/+ 0xCC);/+ 0xD0);/' src/field/effects/particles.c > "$OUT/$mutant.c"
    else
        sed 's/addPrim(ot + value,/addPrim(ot + value * 2,/' src/field/effects/particles.c > "$OUT/$mutant.c"
    fi
    gcc "${COMMON[@]}" "${GAME[@]}" -O2 -fpermissive -w \
        -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.copy.o" \
        "$OUT/O2.test.o" "$OUT/O2.cpu.o" "$OUT/O2.LIBGTE.C.o" \
        "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -o "$OUT/$mutant.test"
    if timeout 10s "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "PARTICLE RENDER OT MUTANT SURVIVED: $mutant" >&2
        exit 1
    fi
    rg -q '^PARTICLE OT depth=' "$OUT/$mutant.log"
done
echo 'FIELD PARTICLE RENDER OT MUTANTS PASS offset/stride'
