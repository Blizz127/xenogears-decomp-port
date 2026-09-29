#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_attachment_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for s in range(231361,231386):
        f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xae48:0xaf64]).hexdigest()=='74668c6567f1addcc854107a218b19d830fa31e737d2fde330cd918e0927c733'
with open('disc/SLUS_006.64','rb') as f:
    f.seek(0x8001cd6c-0x8000f800);accessors=f.read(28)
assert sha256(accessors).hexdigest()=='566e2d6e0b2853ff969f9a46db2e32ede6b0e59c567b41e6ed566c347157cfc2'
PY
common=(-fno-pie -ffunction-sections -fdata-sections -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
cflags=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.$name.o"
    done
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/sprite_attachment_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -fpermissive -w \
        -c pc_port/src/field_object_overlay.c -o "$OUT/$opt.leaf.o"
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -w -c pc_port/src/work_list_port.c -o "$OUT/$opt.work.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.leaf.o" "$OUT/$opt.work.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1
    cat "$OUT/$opt.log"
done
for mutant in extra angle child gate saved vector getter; do
 field="$OUT/O2.leaf.o"
 work="$OUT/O2.work.o"
 if [ "$mutant" = getter ]; then
  sed '/^WorkListCallback_t WorkListTaskGetTaskCallback(/,/^}/ s/pTask->onTriggerCallback/pTask->onFreeCallback/' pc_port/src/work_list_port.c > "$OUT/$mutant.c"
  gcc "${common[@]}" "${cflags[@]}" -O2 -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
  work="$OUT/$mutant.o"
 else
  case "$mutant" in
   extra) expression='s/position, 0x18/position, 0x14/' ;;
   angle) expression='s/func_800223B0(sprite, (s16)angle)/func_800223B0(sprite, (s16)scale)/' ;;
   child) expression='s/data\[5\]/data[6]/' ;;
   gate) expression='s/data\[0x13\] != 0/data[0x13] == 1/' ;;
   saved) expression='s/(u32)(uintptr_t)previous/0/' ;;
   vector) expression='s/(data + 8)/(data + 6)/' ;;
  esac
  sed "/^void func_801E6E48(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
  gcc "${common[@]}" "${cflags[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
  field="$OUT/$mutant.o"
 fi
 clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" "$field" "$work" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "SPRITE ATTACHMENT mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'SPRITE ATTACHMENT FAIL' "$OUT/$mutant.log"
done
echo 'SPRITE ATTACHMENT negative controls PASS: extra angle child gate saved vector getter'
