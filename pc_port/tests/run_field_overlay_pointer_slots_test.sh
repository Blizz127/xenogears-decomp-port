#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_overlay_pointer_slots_test
mkdir -p "$OUT"
read -r routine_hash _ < <(dd if=disc/field.bin bs=1 \
    skip=$((0x800a83b4 - 0x8006faf0)) count=$((0x54)) status=none | sha256sum)
test "$routine_hash" = 813fc87d3838c34aac26237f42a57de2a3c83bd9be4b484fe96ce20f48acacd8
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c \
        pc_port/tests/field_overlay_pointer_slots_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c \
        src/field/main/misc5.c -o "$OUT/$opt.body.o"
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c \
        pc_port/src/data_field.c -o "$OUT/$opt.data.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
sed -e 's/extern u32 D_800AFC60;/extern void* D_800AFC60;/' \
    -e 's/extern u32 D_800AFC64;/extern void* D_800AFC64;/' \
    src/field/main/misc5.c > "$OUT/wide-pointer.c"
gcc "${common[@]}" -O2 -fpermissive -w -c "$OUT/wide-pointer.c" -o "$OUT/wide-pointer.o"
clang -no-pie -Wl,--gc-sections "$OUT/wide-pointer.o" "$OUT/O2.test.o" \
    "$OUT/O2.data.o" -o "$OUT/wide-pointer.test"
if "$OUT/wide-pointer.test" > "$OUT/wide-pointer.log" 2>&1; then
    echo 'OVERLAY POINTER mutant survived' >&2; exit 1
fi
rg -q 'OVERLAY POINTER FAIL' "$OUT/wide-pointer.log"
echo 'OVERLAY POINTER negative control PASS host-width load rejected'
