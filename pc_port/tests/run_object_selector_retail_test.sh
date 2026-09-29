#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/object_selector_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xa830:0xa910]).hexdigest()=='15e3cad3fe23b5c63e7fb5311c088884c7e14cd8c6ca101e198b55108815ea48'
assert sha256(b[0xa7f8:0xa830]).hexdigest()=='3196805b21c24b52a7665109676897485a3738c80ddfa2c49076633956c1fef3'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/object_selector_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in scan_limit global_byte self_alias derived_byte mask_shift slot_ten; do
 case "$mutant" in
  scan_limit) range='^u32 func_801E67F8('; expression='s/index < 8/index < 16/' ;;
  global_byte) range='^u32 func_801E6830('; expression='s/index = (u8)D_801E86B0/index = (u16)D_801E86B0/' ;;
  self_alias) range='^u32 func_801E6830('; expression='s/case 0xF9: index = obj\[0x20\]/case 0xF9: index = obj[0x21]/' ;;
  derived_byte) range='^u32 func_801E6830('; expression='s/index = (u8)(obj\[0x20\] \* 2u + 8u)/index = (obj[0x20] * 2u + 8u)/' ;;
  mask_shift) range='^u32 func_801E6830('; expression='s/index \& 31u/index \& 15u/' ;;
  slot_ten) range='^u32 func_801E6830('; expression='s/index = 10/index = 9/' ;;
 esac
 sed "/$range/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/object_selector_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "OBJECT SELECTOR mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'OBJECT SELECTOR FAIL' "$OUT/$mutant.log"
done
echo 'OBJECT SELECTOR negative controls PASS: scan_limit global_byte self_alias derived_byte mask_shift slot_ten'
