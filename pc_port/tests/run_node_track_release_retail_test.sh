#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/node_track_release_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x352c:0x35f4]).hexdigest()=='a0a446eac8e947dd26933b66a3a750963d7bfb05dcf32e07af3abbbdeafc3fad'
assert sha256(b[0x3e8c:0x3f78]).hexdigest()=='c6a3090588554602e9bb56ccb10bfd96dcedb713542115a7eca6cc2adac7075a'
assert sha256(b[0x37a8:0x37f4]).hexdigest()=='8f70f0f618f542a463cf5615e738dde96473747f1eb56ebce732f2318e3ea393'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/node_track_release_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in signed_index mask single_clear protected root tree_clear; do
 case "$mutant" in
  signed_index) range='^void func_801DF52C('; expression='s/if (index </if ((u32)index </' ;;
  mask) range='^void func_801DF52C('; expression='s/1u << channel/2u << channel/' ;;
  single_clear) range='^void func_801DF52C('; expression='s/\*slot = 0;/*slot = *slot;/' ;;
  protected) range='^void func_801DFE8C('; expression='s/track\[3\] != 0xFF/track[3] != 0xFE/' ;;
  root) range='^void func_801DFE8C('; expression='s/u32 i = 0/u32 i = 1/' ;;
  tree_clear) range='^void func_801DFE8C('; expression='s/\*slot = 0;/*slot = *slot;/' ;;
 esac
 sed "/$range/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/node_track_release_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "NODE TRACK RELEASE mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'NODE TRACK RELEASE FAIL' "$OUT/$mutant.log"
done
echo 'NODE TRACK RELEASE negative controls PASS: signed_index mask single_clear protected root tree_clear'
