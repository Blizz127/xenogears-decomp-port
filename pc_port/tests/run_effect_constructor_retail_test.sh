#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/effect_constructor_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x4a00:0x5258]).hexdigest()=='4ede271950970cf43e61e2864660eaa9a27bbd4145f191c81330dab80d6ec32c'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/effect_constructor_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in active type width allocation carry source callback; do
 case "$mutant" in
  active) expression='s/!= 0) return NULL/== 0) return NULL/' ;;
  type) expression='s/type = (u16)typeArg/type = (u8)typeArg/' ;;
  width) expression='s/half \* 2/half * 1/' ;;
  allocation) expression='s/0x100u << i/0x200u << i/' ;;
  carry) expression='s/carry = (u16)incomingS1/carry = 0/' ;;
  source) expression='s/) \* 6u/) * 8u/' ;;
  callback) expression='s/= callback;/= 0;/' ;;
 esac
 sed "/^u8\* PcPort_FieldEffectConstructWithS1(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/effect_constructor_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "EFFECT CONSTRUCTOR mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'EFFECT CONSTRUCTOR FAIL' "$OUT/$mutant.log"
done
echo 'EFFECT CONSTRUCTOR negative controls PASS: active type width allocation carry source callback'
