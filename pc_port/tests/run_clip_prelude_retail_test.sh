#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/clip_prelude_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x79f0:0x7d34]).hexdigest()=='ca040a2fd3119e90db71fdd0b4a12c4d7af5b560da6766ce1735fcfafa28a364'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/clip_prelude_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/field_clip_prelude.c -o "$OUT/$opt.native.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" "$OUT/$opt.native.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in tick rotation acceleration distance height timer target; do
 case "$mutant" in
  tick) expression='s/ticks == 0/ticks <= 0/' ;;
  rotation) expression='s/>> 3/>> 2/' ;;
  acceleration) expression='s/0x82 + axis/0x76 + axis/' ;;
  distance) expression='s/>= distance/< distance/' ;;
  height) expression='s/= height;/= height + 1;/' ;;
  timer) expression='s/>= limit/> limit/' ;;
  target) expression='s/!= 0) func_801E63A8/== 0) func_801E63A8/' ;;
 esac
 sed "$expression" pc_port/src/field_clip_prelude.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -Wall -Wextra -Werror -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" "$OUT/$mutant.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "CLIP PRELUDE mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'CLIP PRELUDE FAIL' "$OUT/$mutant.log"
done
echo 'CLIP PRELUDE negative controls PASS: tick rotation acceleration distance height timer target'
