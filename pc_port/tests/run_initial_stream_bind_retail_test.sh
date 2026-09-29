#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/initial_stream_bind_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x3ac4:0x3e8c]).hexdigest()=='91826341a3950ea20ff85e1ca4ea296f0d4056dab6df4a6b471daab2a3570f31'
assert sha256(b[0x2f10:0x30b4]).hexdigest()=='e12a4093ee078d91cb5fbfd8699e8ced3af26cd2f1fe848bacc52b1e57bef7d4'
assert sha256(b[0x3e8c:0x3f78]).hexdigest()=='c6a3090588554602e9bb56ccb10bfd96dcedb713542115a7eca6cc2adac7075a'
assert sha256(b[0x36f0:0x37a8]).hexdigest()=='0d6bcac846b80f45722d187b804bc919c4e49af476b1a534a0eaea325e7980a1'
assert sha256(b[0x37a8:0x37f4]).hexdigest()=='8f70f0f618f542a463cf5615e738dde96473747f1eb56ebce732f2318e3ea393'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/initial_stream_bind_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in skipped_cursor exhausted_pose root protected sign frames dirty; do
 case "$mutant" in
  skipped_cursor) expression='s/initialPose += 3;/initialPose += 0;/' ;;
  exhausted_pose) expression='/++consumed\[channel\];/,/if (channel)/ s/if (eligible) {/if (eligible \&\& track != NULL) {/' ;;
  root) expression='s/i != 0/1/' ;;
  protected) expression='s/track\[3\] == 0xFF/track[3] == 0xFE/' ;;
  sign) expression='s/(s16)\*initialPose++/(u16)*initialPose++/g' ;;
  frames) expression='s/if (!loop) --frames;/if (0) --frames;/' ;;
  dirty) expression='s/node\[5\] = 1;/node[5] = 0;/' ;;
 esac
 sed "/^u32 func_801DFAC4(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/initial_stream_bind_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "INITIAL STREAM BIND mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'INITIAL STREAM BIND FAIL' "$OUT/$mutant.log"
done
echo 'INITIAL STREAM BIND negative controls PASS: skipped_cursor exhausted_pose root protected sign frames dirty'
