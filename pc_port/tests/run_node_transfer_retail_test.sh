#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/node_transfer_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xa578:0xa668]).hexdigest()=='c2e23bce6a66f51de4c9e42c8338d43fe4f10a9f219d430ff78d6ac94f50d812'
assert sha256(b[0x37a8:0x37f4]).hexdigest()=='8f70f0f618f542a463cf5615e738dde96473747f1eb56ebce732f2318e3ea393'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/node_transfer_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in clear flag release children parent; do
 case "$mutant" in
  clear) expression='s/node\[7\] = 0/node[7] = 1/' ;;
  flag) expression='s/destination\[7\] = 1/destination[7] = 0/' ;;
  release) expression='s/func_801DF7A8(pool, track);/(void)track;/' ;;
  children) expression='s/child < count/child + 1 < count/' ;;
  parent) expression='s/== (u32)(uintptr_t)node/== 0/' ;;
 esac
 sed "/^void func_801E6578(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/node_transfer_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "NODE TRANSFER mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'NODE TRANSFER FAIL' "$OUT/$mutant.log"
done
echo 'NODE TRANSFER negative controls PASS: clear flag release children parent'
