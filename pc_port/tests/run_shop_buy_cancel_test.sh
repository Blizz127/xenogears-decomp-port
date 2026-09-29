#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=${SHOP_BUY_CANCEL_OUT:-pc_port/build_native/shop_buy_cancel}
mkdir -p "$OUT"
COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
 -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
 -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 ASan; do
 flags=(-"$mode" -fstack-protector-all)
 if [ "$mode" = ASan ]; then flags=(-O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fstack-protector-all); fi
 for part in misc7 misc8; do
  gcc "${COMMON[@]}" "${flags[@]}" -w -c "src/shop_menu/main/$part.c" -o "$OUT/$mode.$part.o"
 done
 gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/shop_buy_cancel_test.c -o "$OUT/$mode.test.o"
 gcc -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.misc7.o" "$OUT/$mode.misc8.o" "$OUT/$mode.test.o" -o "$OUT/$mode"
 "$OUT/$mode"
done
# Restore the undersized native declaration: ASan must identify the overwrite.
sed 's/u8 sp28\[MAX_ITEMS_IN_VIEW\];/u8 sp28[4];/' src/shop_menu/main/misc8.c > "$OUT/mutant.c"
gcc "${COMMON[@]}" "${flags[@]}" -w -c "$OUT/mutant.c" -o "$OUT/mutant.o"
gcc -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/ASan.misc7.o" "$OUT/mutant.o" "$OUT/ASan.test.o" -o "$OUT/mutant"
rc=0; "$OUT/mutant" > "$OUT/mutant.log" 2>&1 || rc=$?
if [ "$rc" = 0 ] || ! grep -q 'AddressSanitizer: stack-buffer-overflow' "$OUT/mutant.log"; then
 echo 'SHOP BUY CANCEL FAIL: undersized buffer not detected' >&2; exit 1
fi
echo 'SHOP BUY CANCEL MUTANT REJECTED: four-byte caller buffer'
