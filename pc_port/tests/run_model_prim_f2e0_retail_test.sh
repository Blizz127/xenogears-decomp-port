#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/model_prim_f2e0_retail_test
mkdir -p "$OUT"
check_slice() {
    local address="$1" size="$2" expected="$3" actual
    read -r actual _ < <(dd if=disc/SLUS_006.64 bs=1 \
        skip=$((address - 0x8000f800)) count=$((size)) status=none | sha256sum)
    if [ "$actual" != "$expected" ]; then
        echo "MODEL PRIM F2E0 RETAIL SLICE MISMATCH: $address" >&2
        exit 1
    fi
}
check_slice 0x8002f2e0 0x1d4 648b7bb0e8ca0474b0cd2fd2f44fa006d965ac3475a4a8239e3625ec4032a540
check_slice 0x8002e1f4 0x38 b7d98d849c2248e0da88f393f8e57a1f9b42df3c4d568ab3c194b01b91f5e2de
check_slice 0x8004fe78 0x28 f170e816fee8ecc43deafc02e7b3c7104bc63edb9ff325d79230cb3b209502c0
check_slice 0x8002c700 0x1e4 42c0125a5e9fc6057382f80fd56c19f1e404c38f41ae1e32a1bbdbf7f6e5b4db

sed -n '/^s32 func_8002F2E0(/,/^}/p' src/slus_006.64/system/temp2.c > "$OUT/body.c"
if ! rg -q '^s32 func_8002F2E0\(' "$OUT/body.c"; then
    echo 'MODEL PRIM F2E0 FAIL missing production owner' >&2
    exit 1
fi
sed -n \
    -e '/^extern u8\* D_80059424;/p' \
    -e '/^extern u32 D_8005953C;/p' \
    -e '/^extern u32 D_80059498;/p' \
    -e '/^extern s32 D_80050100;/p' \
    -e '/^extern s32 D_800500F8;/p' \
    -e '/^extern s32 D_800500FC;/p' \
    -e '/^extern u32 D_80059568;/p' \
    -e '/^extern s32 D_80059578;/p' \
    src/slus_006.64/system/temp2.c | awk '!seen[$0]++' > "$OUT/globals.h"
test "$(wc -l < "$OUT/globals.h")" = 8
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
    for pair in "test:pc_port/tests/model_prim_f2e0_retail_test.c" \
                "cpu:pc_port/src/battle_mips_adapter.c" \
                "body:src/slus_006.64/system/temp2.c" \
                "model_link:pc_port/src/model_prim_link.c" \
                "guest_link:pc_port/src/guest_prim_link.c"; do
        name=${pair%%:*}; source=${pair#*:}
        if [ "$name" = body ]; then
            # Compile the actual game TU with its own includes. Pre-including
            # PsyCross's inline_c.h into an extracted body hid a native build
            # that emitted PsyQ's MIPS assembler placeholders into x86 code.
            gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -fpermissive -w \
                -c "$source" -o "$OUT/$opt.$name.o"
            continue
        fi
        gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -Wall -Wextra -Werror \
            -include common.h -include psyq/libgte.h -include psx/inline_c.h \
            -include psx_memory.h -include pc_port/src/model_prim_link.h \
            -include "$OUT/globals.h" \
            -c "$source" -o "$OUT/$opt.$name.o"
    done
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -fpermissive -w \
        -c pc_port/src/psyq_compat.c -o "$OUT/$opt.compat.o"
    gcc "${COMMON[@]}" "${GAME[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$opt.leaf.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -ldl \
        -o "$OUT/$opt.test"
    echo "MODEL PRIM F2E0 full production TU $opt"
    timeout 30s "$OUT/$opt.test"
done
for mutant in terminal_rtpt normal_v0 partial_xy packed_v1; do
    case "$mutant" in
        terminal_rtpt)
            sed 's/gte_rtpt();/if (remaining != 0) gte_rtpt();/' "$OUT/body.c" > "$OUT/$mutant.c" ;;
        normal_v0)
            sed 's/MTC2(\*(u32\*)(normals - 8), 0);/MTC2(0, 0);/' \
                "$OUT/body.c" > "$OUT/$mutant.c" ;;
        partial_xy)
            sed 's/\*(u32\*)(out + 0x10) = xy1;/\/\* mutant omits delay-slot XY1 write *\//' \
                "$OUT/body.c" > "$OUT/$mutant.c" ;;
        packed_v1)
            sed 's/(cmd >> 13) & 0xfff8u/(cmd >> 16) << 3/' \
                "$OUT/body.c" > "$OUT/$mutant.c" ;;
    esac
    gcc "${COMMON[@]}" "${GAME[@]}" -O2 -Wall -Wextra -Werror \
        -include common.h -include psyq/libgte.h -include psx/inline_c.h \
        -include psx_memory.h -include pc_port/src/model_prim_link.h \
        -include "$OUT/globals.h" -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.model_link.o" "$OUT/O2.guest_link.o" \
        "$OUT/O2.compat.o" "$OUT/O2.leaf.o" "$OUT/O2.LIBGTE.C.o" \
        "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$OUT/$mutant.test"
    if timeout 30s "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "MODEL PRIM F2E0 MUTANT SURVIVED: $mutant" >&2
        exit 1
    fi
    rg -q 'MODEL PRIM F2E0 FAIL' "$OUT/$mutant.log"
done
echo 'MODEL PRIM F2E0 MUTANTS PASS terminal-RTPT/normal-V0/partial-XY/packed-v1'
