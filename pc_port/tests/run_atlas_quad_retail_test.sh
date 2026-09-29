#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/atlas_quad_retail_test
mkdir -p "$OUT"
read -r hash _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x80025fa8-0x8000f800)) count=$((0x390)) status=none | sha256sum)
test "$hash" = a0f6f0de866dd0c2b8d4f7ae0733a3ba4b5586be3a4bb5ca2f49dd96b0fb5b78
read -r hash _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x800188cc-0x8000f800)) count=32 status=none | sha256sum)
test "$hash" = 9f6665a8cdb2980ee185f3e931a1872ef4400ec18f79fd05036db36fbe7a1278
read -r hash _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x8004fdc0-0x8000f800)) count=32 status=none | sha256sum)
test "$hash" = 50fafc982a2c039a6e831df3e512a208cae6e222cd4423a3d070c78d567bf9b4
read -r hash _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x80043a1c-0x8000f800)) count=$((0x2a8)) status=none | sha256sum)
test "$hash" = fb063ed80a14270d731dc1f70fe90a5d32e9d79dc9b5c93796ffcab9a72072d8
sed -n '/^s32 func_80025FA8(/,/^}/p' src/slus_006.64/system/temp1f.c > "$OUT/body.c"
sed -n -e '/^u_short GetClut(/,/^}/p' -e '/^u_short GetTPage(/,/^}/p' \
    -e '/^void SetSemiTrans(/,/^}/p' -e '/^void SetPolyFT4(/,/^}/p' \
    -e '/^void SetShadeTex(/,/^}/p' pc_port/extern/PsyCross/src/psx/LIBGPU.C > "$OUT/gpu.c"
COMMON=(-fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/build_native)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${COMMON[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.$name.o"
    done
    for pair in "test:pc_port/tests/atlas_quad_retail_test.c" "cpu:pc_port/src/battle_mips_adapter.c" \
        "body:$OUT/body.c" "gpu:$OUT/gpu.c"; do
        name=${pair%%:*}; source=${pair#*:}
        gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -Wall -Wextra -Werror \
            -include common.h -include psyq/libgpu.h -include psyq/libgte.h -include psx_memory.h \
            -c "$source" -o "$OUT/$opt.$name.o"
    done
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -fpermissive -w \
        -c pc_port/src/psyq_compat.c -o "$OUT/$opt.compat.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$opt.leaf.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -ldl -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in buffer uv; do
    if [ "$mutant" = buffer ]; then
        sed 's/buffer \* 40/buffer * 80/' "$OUT/body.c" > "$OUT/$mutant.c"
    else
        sed 's/if (rotation == 0xc00) u--;/if (rotation == 0xc00) u += 0;/' "$OUT/body.c" > "$OUT/$mutant.c"
    fi
    gcc "${COMMON[@]}" "${GAME[@]}" -O2 -Wall -Wextra -Werror \
        -include common.h -include psyq/libgpu.h -include psyq/libgte.h -include psx_memory.h \
        -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.gpu.o" "$OUT/O2.compat.o" "$OUT/O2.leaf.o" \
        "$OUT/O2.LIBGTE.C.o" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "ATLAS MUTANT SURVIVED: $mutant" >&2; exit 1
    fi
    rg -q 'ATLAS QUAD FAIL fixture=' "$OUT/$mutant.log"
done
echo 'ATLAS QUAD MUTANTS PASS buffer/UV'
