#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=pc_port/build_native/system_data_lookup_retail_test
mkdir -p "$OUT"
read -r retail_sha _ < <(dd if=disc/SLUS_006.64 bs=1 \
    skip=$((0x80033728 - 0x8000f800)) count=$((0x5a8)) status=none | sha256sum)
if [ "$retail_sha" != b4b0df02ca161eee5c67dded72a1615df824ee51074cf15f2ec8a5ecfbcff4f2 ]; then
    echo "SYSTEM DATA RETAIL SLICE MISMATCH" >&2
    exit 1
fi
COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w \
        -c src/slus_006.64/system/system.c -o "$OUT/$opt.system.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/system_data_lookup_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.system.o" \
        "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    timeout 15s "$OUT/$opt.test"
done
