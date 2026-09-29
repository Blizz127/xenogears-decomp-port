#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/shared_transform_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for s in range(231361,231386):
        f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x5880:0x5a14]).hexdigest()=='66166223611c2503d903567f301fe314e8a346c3124b4a4cf1f3856589fa2be9'
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
        -c pc_port/tests/shared_transform_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -fpermissive -w \
        -c pc_port/src/field_object_overlay.c -o "$OUT/$opt.leaf.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.leaf.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1
    cat "$OUT/$opt.log"
done
for mutant in count active slot child vector command fallback; do
    case "$mutant" in
        count) expression='s/index < 2/index < 1/' ;;
        active) expression='s/if ((s16)record\[3\] == 0) continue;/if (0) continue;/' ;;
        slot) expression='s/slot < 0 ? NULL/slot < 0 || slot >= 10 ? NULL/' ;;
        child) expression='s/+ 0xA8u/+ 0x2Cu/' ;;
        vector) expression='s/record + 4, 4/record + 5, 4/' ;;
        command) expression='s/0x00480012/0x00486012/' ;;
        fallback) expression='s/record\[0\] = record\[4\];/record[0] = 0;/' ;;
    esac
    sed "/^void func_801E1880(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" "${cflags[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" \
        "$OUT/$mutant.o" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "SHARED TRANSFORM mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'SHARED TRANSFORM FAIL' "$OUT/$mutant.log"
done
echo "SHARED TRANSFORM negative controls PASS: count active slot child vector command fallback"
