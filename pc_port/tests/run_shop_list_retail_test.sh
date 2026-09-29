#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${SHOP_LIST_OUT:-pc_port/build_native/shop_list}
mkdir -p "$OUT"
test "$(sha256sum disc/shop_menu.bin | cut -d' ' -f1)" = 7890e14bcabddcf85368de10783ecff8daa166dc5ac254e06db5e27959cab4cf
COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
 -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
 -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode"); if [ "$mode" = UBSan ]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c src/shop_menu/main/misc7.c -o "$OUT/$mode.source.o"
 gcc "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/shop_list_retail_test.c -o "$OUT/$mode.test.o"
 gcc "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 gcc -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
 "$OUT/$mode" disc/shop_menu.bin | tee "$OUT/$mode.log"
done
for mutation in affordability row-count price-digit context; do
 python3 - "$mutation" "$OUT/mutant.c" <<'PY'
import sys
from pathlib import Path
s=Path('src/shop_menu/main/misc7.c').read_text()
a,b={
 'affordability':('gold >= price','gold > price'),
 'row-count':('row < 8','row < 7'),
 'price-digit':('remaining % 10 + 0x10','remaining % 10 + 0x11'),
 'context':('name->renderContext = (u8)g_Menu->renderContext;\n        name->renderContext', 'name->renderContext = (u8)g_Menu->renderContext;\n        cost->renderContext'),
}[sys.argv[1]]
assert a in s
Path(sys.argv[2]).write_text(s.replace(a,b))
PY
 gcc "${COMMON[@]}" -O2 -fpermissive -w -c "$OUT/mutant.c" -o "$OUT/mutant.o"
 gcc -no-pie -O2 -Wl,--gc-sections "$OUT/mutant.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/mutant"
 rc=0; "$OUT/mutant" disc/shop_menu.bin > "$OUT/$mutation.log" 2>&1 || rc=$?
 if [ "$rc" = 0 ] || ! grep -q '^SHOP LIST FAIL' "$OUT/$mutation.log"; then
  echo "SHOP LIST MUTANT SURVIVED: $mutation" >&2; exit 1
 fi
 echo "SHOP LIST MUTANT REJECTED: $mutation"
done
