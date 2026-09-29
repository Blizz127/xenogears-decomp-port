#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/trail_init_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xc510:0xc590]).hexdigest()=='0764da3a5f31586321a67a62de035066a3c1694ce28e81ecec92308e09dc580d'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/trail_init_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in zero size sentinel offset reload; do
 case "$mutant" in
  zero) expression='s/if (parent\[0x10C\] != 0)/if (1)/' ;;
  size) expression='s/\* 0x70u/* 0x6Cu/' ;;
  sentinel) expression='s/= -1;/= 0;/' ;;
  offset) expression='s/0x70 + 8/0x70 + 4/' ;;
  reload) expression='s/u8\* entries =/u32 savedCount = parent[0x10C]; u8* entries =/; s/i < parent\[0x10C\]/i < savedCount/' ;;
 esac
 sed "/^void func_801E8510(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/trail_init_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "TRAIL INIT mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'TRAIL INIT FAIL' "$OUT/$mutant.log"
done
echo 'TRAIL INIT negative controls PASS: zero size sentinel offset reload'
