#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_constructor_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
with open('disc/SLUS_006.64','rb') as f:
 f.seek(0x80023fd8-0x8000f800);b=f.read(700)
assert sha256(b).hexdigest()=='6291763a42ce40bf3073a031287166b8401aa0e6655b0cd4a47cfad702393c65'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/sprite_constructor_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in entry gate inherit reset position type duration; do
 case "$mutant" in
  entry) expression='s/entry + 2u/entry + 4u/' ;;
  gate) expression='s/D_800591AD != 0/D_800591AD == 1/' ;;
  inherit) expression='s/parent + 0x18/parent + 0x1C/' ;;
  reset) expression='s/write32(sprite + 0x44, 0);/write32(sprite + 0x44, 1);/' ;;
  position) expression='s/<< 16/<< 15/' ;;
  type) expression='s/type \& 0xFu/type \& 7u/' ;;
  duration) expression='s/(u16)D_800591A8/(u16)0/' ;;
 esac
 sed "/^u8\* func_80023FD8(/,/^}/ { $expression; }" pc_port/src/sprite_constructor.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DSPRITE_CONSTRUCTOR_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/sprite_constructor_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "SPRITE CONSTRUCTOR mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'SPRITE CONSTRUCTOR FAIL' "$OUT/$mutant.log"
done
echo 'SPRITE CONSTRUCTOR negative controls PASS: entry gate inherit reset position type duration'
