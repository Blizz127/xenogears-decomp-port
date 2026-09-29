#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/linked_geometry_constructor_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x5a14:0x62f8]).hexdigest()=='d2f25c7c9728b4929a214cf8dbc4db756e45d4d0ca75d405356eae1a130f3f03'
with open('disc/SLUS_006.64','rb') as f:
 f.seek(0x80043a1c-0x8000f800)
 assert sha256(f.read(0x300)).hexdigest()=='d7e5c65695cfdae806e08f4a419ba9d4fc7e1503da10abb8d728fa7e71958ad0'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/linked_geometry_constructor_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in signed_anchor null_free length sentinel uv color horizontal_fault vertical_fault; do
 case "$mutant" in
  signed_anchor) expression='s/(u32)\*source++/(u32)(s32)(s16)*source++/' ;;
  null_free) expression='s/HeapFree(NULL);/HeapFree(anchors);/' ;;
  length) expression='s/(u32)\*lengths++/(u32)(s32)(s16)*lengths++/' ;;
  sentinel) expression='s/\*(u16\*)vertex = 0;/*(u16*)vertex = 1;/' ;;
  uv) expression='s/j + tri/j/' ;;
  color) expression='s/owner\[0xF\] = (u8)backR/owner[0xF] = (u8)r/' ;;
  horizontal_fault) expression='s/s32 step =/if (*(s16*)(owner + 4) == 1) return; s32 step =/' ;;
  vertical_fault) expression='s/u32 vstep =/if (pairs == 0) return; u32 vstep =/' ;;
 esac
 sed "/^void func_801E1A14(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/linked_geometry_constructor_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "GEOMETRY CONSTRUCTOR mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'GEOMETRY CONSTRUCTOR FAIL' "$OUT/$mutant.log"
done
echo 'GEOMETRY CONSTRUCTOR negative controls PASS: signed_anchor null_free length sentinel uv color horizontal_fault vertical_fault'
