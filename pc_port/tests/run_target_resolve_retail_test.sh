#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/target_resolve_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for s in range(231361,231386):
        f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xa3a8:0xa578]).hexdigest()=='4ddc077241fbfd1dea89eb13ff3500e1f20af9b3c45c4f70c32fd4d239e3973d'
PY
common=(-fno-pie -ffunction-sections -fdata-sections -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
cflags=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.$name.o"
    done
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/target_resolve_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -fpermissive -w \
        -c pc_port/src/field_object_overlay.c -o "$OUT/$opt.leaf.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.leaf.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1
    cat "$OUT/$opt.log"
done
for mutant in scan selector self child command mode output; do
 case "$mutant" in
  scan) expression='s/slot < 8/slot < 7/' ;;
  selector) expression='s/slot = (u32)selector - 1u/slot = (u32)selector/' ;;
  self) expression='s/ || slot == object\[0x20\]//' ;;
  child) expression='s/+ 0x2Cu/+ 0x30u/' ;;
  command) expression='s/0x00480012/0x00486012/' ;;
  mode) expression='s/- 7u < 2u/- 7u < 1u/' ;;
  output) expression='s/object + 0x88/object + 0x8A/' ;;
 esac
 sed "/^void func_801E63A8(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" "${cflags[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" "$OUT/$mutant.o" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "TARGET RESOLVE mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'TARGET RESOLVE FAIL' "$OUT/$mutant.log"
done
echo 'TARGET RESOLVE negative controls PASS: scan selector self child command mode output'
