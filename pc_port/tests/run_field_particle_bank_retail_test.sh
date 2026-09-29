#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=pc_port/build_native/field_particle_bank_retail_test
mkdir -p "$OUT"
read -r retail_sha _ < <(dd if=disc/field.bin bs=1 \
    skip=$((0x800a9274 - 0x8006faf0)) count=$((0x8a8)) status=none | sha256sum)
if [ "$retail_sha" != 1c5e78294b5415ce389b270d92aea46878e8a42474b2e53a935a0c21acad6da4 ]; then
    echo "FIELD PARTICLE BANK RETAIL SLICE MISMATCH" >&2
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
        -c src/field/effects/particles.c -o "$OUT/$opt.native.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/field_particle_bank_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.native.o" \
        "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    timeout 20s "$OUT/$opt.test"
done
