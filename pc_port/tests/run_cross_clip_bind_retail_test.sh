#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/cross_clip_bind_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xc394:0xc430]).hexdigest()=='52f01bdbf34869a3e7cf2cd2704d57bee4a84dfcd0376e1ee33ef71eff93ee1f'
assert sha256(b[0x75d0:0x76bc]).hexdigest()=='7b59dc488b040e5b2b64a898276871a668e7a1795002fb826e140ac020ddfb9b'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/cross_clip_bind_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -DXENO_TEST_OVERLAY_UNIT -c pc_port/tests/cross_clip_bind_retail_test.c -o "$OUT/$opt.overlay.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.overlay.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in slot mask clear gate source animation; do
 case "$mutant" in
  slot) expression='s/u32 index = (u16)slot;/u32 index = (u16)slot % 8;/' ;;
  mask) expression='s/D_801E863C = (s16)mask;/D_801E863C = 0;/' ;;
  clear) expression='s/object\[0x35\] = 0;/object[0x35] = 1;/' ;;
  gate) expression='s/index != (u32)func_801E67F8()/index == (u32)func_801E67F8()/' ;;
  source) expression='s/object, source, D_801E86A8/object, object, D_801E86A8/' ;;
  animation) expression='s/D_801E86A8, animation/D_801E86A8, 0/' ;;
 esac
 sed "/^void func_801E8394(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w -DXENO_TEST_OVERLAY_UNIT "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/cross_clip_bind_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "CROSS CLIP mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'CROSS CLIP FAIL' "$OUT/$mutant.log"
done
echo 'CROSS CLIP negative controls PASS: slot mask clear gate source animation'
