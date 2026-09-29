#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/rotation_bind_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x99d4:0x9b50]).hexdigest()=='312c073a2f1d21558e69536b6472742d75a21ccb022cb0f48b69b63729aeba82'
assert sha256(b[0x36f0:0x37a8]).hexdigest()=='0d6bcac846b80f45722d187b804bc919c4e49af476b1a534a0eaea325e7980a1'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/rotation_bind_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in duration dirty tag shortest elapsed; do
 case "$mutant" in
  duration) expression='s/duration < 2/duration < 1/' ;;
  dirty) expression='s/node\[5\] = 1/node[4] = 1/' ;;
  tag) expression='s/track\[3\] = 0xFE/track[3] = 0xFF/' ;;
  shortest) expression='s/delta >= 0x800/delta > 0x800/' ;;
  elapsed) expression='s/\*(u16\*)(track + 0x10) = 0/*(u16*)(track + 0x10) = 1/' ;;
 esac
 sed "/^void func_801E59D4(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/rotation_bind_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "ROTATION BIND mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'ROTATION BIND FAIL' "$OUT/$mutant.log"
done
echo 'ROTATION BIND negative controls PASS: duration dirty tag shortest elapsed'
