#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=pc_port/build_native/field_particle_primitive_retail_test
mkdir -p "$OUT"
check_slice() {
    local file="$1" base="$2" address="$3" size="$4" expected="$5" actual
    read -r actual _ < <(dd if="$file" bs=1 skip=$((address - base)) count=$((size)) status=none | sha256sum)
    if [ "$actual" != "$expected" ]; then
        echo "PARTICLE PRIMITIVE RETAIL SLICE MISMATCH: $file $address" >&2
        exit 1
    fi
}
check_slice disc/field.bin 0x8006faf0 0x800a8eac 0x208 7fa217a406233868cd5c85d076009e48f522c4476a290ec87954a2bd63e5b203
check_slice disc/field.bin 0x8006faf0 0x8007a44c 0x178 c9ea0445847f175c48c6319e55528bdc6cb928747fcae4fd79e286fd74919c3c
check_slice disc/field.bin 0x8006faf0 0x800af27c 0x200 9b8952b872ecf0d3c52d255fe6bb72c4002efeb518fc03b94c212c1da2760937
check_slice disc/SLUS_006.64 0x8000f800 0x80043a1c 0x2a8 fb063ed80a14270d731dc1f70fe90a5d32e9d79dc9b5c93796ffcab9a72072d8

# Compile exact production GPU leaf bodies without the desktop/GL lifecycle.
# The retail side executes all four corresponding EXE leaves, without bridges.
sed -n -e '/^u_short GetClut(/,/^}/p' -e '/^u_short GetTPage(/,/^}/p' \
    -e '/^void SetSemiTrans(/,/^}/p' -e '/^void SetPolyFT4(/,/^}/p' \
    pc_port/extern/PsyCross/src/psx/LIBGPU.C > "$OUT/gpu_leaves.c"
test "$(rg -c '^(u_short|void) ' "$OUT/gpu_leaves.c")" = 4
COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in src/field/main/misc5.c src/field/main/misc4.c pc_port/src/data_field.c; do
        name=${source##*/}
        gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c "$source" -o "$OUT/$opt.$name.o"
    done
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -include common.h -include field/particles.h \
        -c "$OUT/gpu_leaves.c" -o "$OUT/$opt.gpu.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/field_particle_primitive_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.misc5.c.o" \
        "$OUT/$opt.misc4.c.o" "$OUT/$opt.data_field.c.o" "$OUT/$opt.gpu.o" \
        "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    timeout 30s "$OUT/$opt.test"
done
