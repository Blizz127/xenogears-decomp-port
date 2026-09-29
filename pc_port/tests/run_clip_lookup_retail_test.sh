#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/clip_lookup_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xa910:0xa974]).hexdigest()=='8419a785639aafafb67573a38a3fb70a9efbdee1ecb90d7cbc798c17011f8fd2'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/clip_lookup_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in output_clear default_gate flag_value table_boundary primary_offset secondary_offset; do
 case "$mutant" in
  output_clear) expression='s/\*flags = 0;/*flags = *flags;/' ;;
  default_gate) expression='s/(u8)index >= 0xFE/(u8)index > 0xFE/' ;;
  flag_value) expression='s/\*flags = obj\[0x2A\] \& 0x80;/*flags = (obj[0x2A] \& 0x80) != 0;/' ;;
  table_boundary) expression='s/selected < 0x40/selected < 0x41/' ;;
  primary_offset) expression='s/selected \* 4u + 4u/selected * 4u/' ;;
  secondary_offset) expression='s/0xFCu/0xF8u/' ;;
 esac
 sed "/^u32 func_801E6910(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/clip_lookup_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "CLIP LOOKUP mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'CLIP LOOKUP FAIL' "$OUT/$mutant.log"
done
echo 'CLIP LOOKUP negative controls PASS: output_clear default_gate flag_value table_boundary primary_offset secondary_offset'
