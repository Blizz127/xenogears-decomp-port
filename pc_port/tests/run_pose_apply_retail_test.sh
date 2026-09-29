#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/pose_apply_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x2f10:0x30b4]).hexdigest()=='e12a4093ee078d91cb5fbfd8699e8ced3af26cd2f1fe848bacc52b1e57bef7d4'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/pose_apply_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in prefix count rotation_gate translation_gate rotation_dirty translation_dirty comparison sign; do
 case "$mutant" in
  prefix) expression='s/if (prefix == 0)/if (0)/' ;;
  count) expression='s/u32 children =/if (*(u16*)(root + 0xA) > 64) return; u32 children =/' ;;
  rotation_gate) expression='s/node + 0x70/node + 0x74/' ;;
  translation_gate) expression='s/node + 0x74/node + 0x70/' ;;
  rotation_dirty) expression='s/node\[5\] = 1;/node[5] = 0;/' ;;
  translation_dirty) expression='s/node\[4\] = 1;/node[4] = 1; node[5] = 1;/' ;;
  comparison) expression='s/if (\*(s16\*)(node + 0x54) != x/if (1 || *(s16*)(node + 0x54) != x/' ;;
  sign) expression='s/s32 x = \*source++, y = \*source++, z = \*source++;/s32 x = (u16)*source++, y = (u16)*source++, z = (u16)*source++;/' ;;
 esac
 sed "/^void func_801DEF10(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/pose_apply_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "POSE APPLY mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'POSE APPLY FAIL' "$OUT/$mutant.log"
done
echo 'POSE APPLY negative controls PASS: prefix count rotation_gate translation_gate rotation_dirty translation_dirty comparison sign'
