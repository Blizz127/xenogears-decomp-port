#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_attach_callback_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for s in range(231361,231386):
        f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xaf64:0xb094]).hexdigest()=='641195ca393cf6c7f11befe4ad49bd40aa20a7b045b327f0b9a1a393af3f75b4'
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
        -c pc_port/tests/sprite_attach_callback_retail_test.c -o "$OUT/$opt.test.o"
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
for mutant in child vector command floor shift callback; do
 case "$mutant" in
  child) expression='s/+ 0x2Cu/+ 0x30u/' ;;
  vector) expression='s/attachment + 0x10/attachment + 0x14/' ;;
  command) expression='s/0x00480012/0x00486012/' ;;
  floor) expression='s/(attachment + 0xE) != 0/(attachment + 0xE) == 1/' ;;
  shift) expression='s/position\[axis\] << 16/position[axis] << 15/' ;;
  callback) expression='s/PcPort_WorkListInvokeSavedCallback(\*(u32\*)(attachment + 4), wrapper);/(void)wrapper;/' ;;
 esac
 sed "/^void func_801E6F64(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
 gcc "${common[@]}" "${cflags[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" "$OUT/$mutant.o" "$OUT/O2.work.o" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "SPRITE ATTACH CALLBACK mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'SPRITE ATTACH CALLBACK FAIL' "$OUT/$mutant.log"
done
echo 'SPRITE ATTACH CALLBACK negative controls PASS: child vector command floor shift callback'
