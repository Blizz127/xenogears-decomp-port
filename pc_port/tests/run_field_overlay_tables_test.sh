#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_overlay_tables_test
mkdir -p "$OUT"
read -r table_hash _ < <(dd if=disc/field.bin bs=1 \
    skip=$((0x800aeb68 - 0x8006faf0)) count=$((0x710)) status=none | sha256sum)
test "$table_hash" = 0dff25f38e14a98cb98a48bfced5e53b75b4bd9f860ff650a5b824fe58050e57
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c \
        pc_port/tests/field_overlay_tables_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c \
        pc_port/src/data_field.c -o "$OUT/$opt.data.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in bytes alias; do
    if [ "$mutant" = bytes ]; then
        # The table is loaded from disc/field.bin (retail_data.h): shift its
        # load row by one halfword so every loaded value is wrong.
        sed 's/XENO_RD(g_FieldOverlayLayoutData, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AEB68u)/XENO_RD(g_FieldOverlayLayoutData, XENO_RD_FIELD, XENO_RD_FIELD_OFF(0x800AEB6Au)/' \
            pc_port/src/data_field.c > "$OUT/$mutant.c"
        grep -q '0x800AEB6Au' "$OUT/$mutant.c"
    else
        sed 's/g_FieldOverlayLayoutData + 0x3AE/g_FieldOverlayLayoutData + 0x3AC/' \
            pc_port/src/data_field.c > "$OUT/$mutant.c"
    fi
    gcc "${common[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "OVERLAY TABLE mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'OVERLAY TABLE FAIL' "$OUT/$mutant.log"
done
echo 'OVERLAY TABLE negative controls PASS byte corruption/interior alias'
