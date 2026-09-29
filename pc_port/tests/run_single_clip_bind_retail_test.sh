#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/single_clip_bind_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
 for s in range(231361,231386):
  f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xc330:0xc394]).hexdigest()=='338db1f71d21b868721a7760e971ef414800b2eb601df4ccc0f34f8f28da8763'
assert sha256(b[0x75d0:0x76bc]).hexdigest()=='7b59dc488b040e5b2b64a898276871a668e7a1795002fb826e140ac020ddfb9b'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -Ipc_port/include_shim -Iinclude -Ipc_port/src
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
# The test provides its own recording func_801E39F0; strip the real one from
# the overlay source so the recorder is the sole definition. (The oracle
# path and the missing-interpreter variant below are unaffected.)
sed "/^void func_801E39F0(u8\* obj, void\* context, s32 limit, s32 ticks, s32 mode)$/,/^}/d" \
    pc_port/src/field_object_overlay.c > "$OUT/overlay_main.c"
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/overlay_main.c\"" -c pc_port/tests/single_clip_bind_retail_test.c -o "$OUT/$opt.test.o"
 gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in slot mask clear phase animation context; do
 case "$mutant" in
  slot) expression='s/D_801E86B0 = (s16)slot;/D_801E86B0 = 0;/' ;;
  mask) expression='s/D_801E863C = (s16)mask;/D_801E863C = 0;/' ;;
  clear) expression='s/obj\[0x35\] = 0;/obj[0x35] = 1;/' ;;
  phase) expression='s/obj\[0x35\] = 0;/obj[0x35] = 0; *(u16*)(obj + 0x10a) = (u16)mask;/' ;;
  animation) expression='s/D_801E86A8, anim/D_801E86A8, 0/' ;;
  context) expression='s/obj, obj, D_801E86A8/obj, obj, NULL/' ;;
 esac
 sed "/^void func_801E8330(/,/^}/ { $expression; }" "$OUT/overlay_main.c" > "$OUT/$mutant.c"
 gcc "${common[@]}" -O2 -fpermissive -w "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" -c pc_port/tests/single_clip_bind_retail_test.c -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "SINGLE CLIP mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'SINGLE CLIP FAIL' "$OUT/$mutant.log"
done
echo 'SINGLE CLIP negative controls PASS: slot mask clear phase animation context'
gcc "${common[@]}" -O2 -fpermissive -w -DXENO_TEST_MISSING_VM -c pc_port/tests/single_clip_bind_retail_test.c -o "$OUT/missing.o"
for source in field_clip_control field_clip_data field_clip_prelude; do
 gcc "${common[@]}" -O2 -fpermissive -w -c "pc_port/src/$source.c" -o "$OUT/missing.$source.o"
done
for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
 name=${source##*/}
 g++ -std=c++17 -fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections \
  -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
  -O2 -fpermissive -w -include pc_port/src/port_compat.h \
  -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/missing.$name.o"
done
clang -no-pie -Wl,--gc-sections "$OUT/missing.o" "$OUT/O2.cpu.o" "$OUT/missing.field_clip_control.o" "$OUT/missing.field_clip_data.o" "$OUT/missing.field_clip_prelude.o" "$OUT/missing.LIBGTE.C.o" "$OUT/missing.INLINE_C.C.o" "$OUT/missing.PsyX_GTE.cpp.o" -o "$OUT/missing.test"
# The retail interpreter exists now, so no missing-interpreter abort can be
# observed; the link above remains as coverage that the real TU (with its
# clip/GTE dependencies) links standalone.
echo 'SINGLE CLIP real interpreter links standalone (missing-recorder variant)'
