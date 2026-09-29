#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/script_stop_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x4844:0x4850]).hexdigest()=='4ff83a93b28850a90cc018298227c8bda54d9fb9734eb51f729dec1998e4e0f0'
assert sha256(b[0xa32c:0xa338]).hexdigest()=='e72b17d79ecd6e56890a26389eca4dd0d82b0fff7f30845317c7871ce006ff9c'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/script_stop_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in trail_marker trail_return script_offset script_return; do
 case "$mutant" in
  trail_marker) range='^s32 func_801E0844('; expression='s/= 0xffff;/= 0;/' ;;
  trail_return) range='^s32 func_801E0844('; expression='s/return -1;/return 0;/' ;;
  script_offset) range='^s32 func_801E632C('; expression='s/0x98/0x9a/' ;;
  script_return) range='^s32 func_801E632C('; expression='s/return -1;/return 0;/' ;;
 esac
 sed "/$range/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/script_stop_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "SCRIPT STOP mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'SCRIPT STOP FAIL' "$OUT/$mutant.log"
done
echo 'SCRIPT STOP negative controls PASS: trail_marker trail_return script_offset script_return'
