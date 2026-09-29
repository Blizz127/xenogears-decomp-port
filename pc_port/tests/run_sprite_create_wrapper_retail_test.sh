#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=pc_port/build_native/sprite_create_wrapper_retail_test
mkdir -p "$OUT"
read -r retail_sha _ < <(dd if=disc/SLUS_006.64 bs=1 \
    skip=$((0x800242f4 - 0x8000f800)) count=$((0x68)) status=none | sha256sum)
if [ "$retail_sha" != f771a8dee7eb81c6a485bcd86185b4962dedbb97eb5b1f40d001871b251997bb ]; then
    echo "SPRITE WRAPPER RETAIL SLICE MISMATCH" >&2
    exit 1
fi
# Mechanically extract the unchanged production wrapper region. Its callee is
# defined in the same large TU; isolate that boundary without maintaining a
# copied implementation or modifying the game's source for a test hook.
# temp1.c declares D_800591B8 inside an XENO_PC_PORT/retail #if block (retail
# defines it for %gp_rel), so start the slice after it and declare it here.
{ echo 'extern s32 D_800591B8;'
  sed -n '/^extern void\* func_80024524(/,/^extern u8 D_800591AD;/p' \
    src/slus_006.64/system/temp1.c; } > "$OUT/production_wrappers.c"
test "$(rg -c '^void\* func_800242F4\(' "$OUT/production_wrappers.c")" = 1
COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -include common.h \
        -c "$OUT/production_wrappers.c" -o "$OUT/$opt.native.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/sprite_create_wrapper_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.native.o" \
        "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    timeout 20s "$OUT/$opt.test"
done
