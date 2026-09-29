#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/distance_track_bind_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x9b50:0x9c74]).hexdigest()=='2ed8c6c81c3f3ad28f2ad4b20f6e72d4d34e78d5a93b1dab326ebce2a2cbfd21'
assert sha256(b[0x36f0:0x37a8]).hexdigest()=='0d6bcac846b80f45722d187b804bc919c4e49af476b1a534a0eaea325e7980a1'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/distance_track_bind_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in mode tag axis increment duration; do
 case "$mutant" in
  mode) expression='s/+ 7u/+ 6u/' ;;
  tag) expression='s/track\[3\] = 0xFE/track[3] = 0xFF/' ;;
  axis) expression='s/node + 0x64/node + 0x60/' ;;
  increment) expression='s/+ 1u/+ 2u/' ;;
  duration) expression='s/(u16)duration/(u16)first/' ;;
 esac
 sed "/^void func_801E5B50(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/distance_track_bind_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "DISTANCE TRACK BIND mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'DISTANCE TRACK BIND FAIL' "$OUT/$mutant.log"
done
echo 'DISTANCE TRACK BIND negative controls PASS: mode tag axis increment duration'
