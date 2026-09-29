#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/object_link_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x77d0:0x79f0]).hexdigest()=='00001a8d7b974caec5c8bdf0288340aa367b46d2d89ad899eea2a59682340646'
PY
common=(-fno-pie -ffunction-sections -fdata-sections -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w -include pc_port/src/port_compat.h \
            -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.${source##*/}.o"
    done
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -fpermissive -w \
        -c pc_port/tests/object_link_retail_test.c -o "$OUT/$opt.test.o"
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -ldl -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in detach visibility joint orientation offset duplicate; do
    case "$mutant" in
        detach) expression='s/obj\[0x5C\] = 0xFF/obj[0x5C] = 0/' ;;
        visibility) expression='s/\& 0x10/\& 0x20/' ;;
        joint) expression='s/\*(s16\*)(obj + 0x5E)/0/g' ;;
        orientation) expression='s/obj\[0x5D\] != 0/0/' ;;
        offset) expression='s/obj + 0x6A/obj + 0x6C/' ;;
        duplicate) expression='s/gte_stlvnl(root + 0x5C)/gte_stlvnl(root + 0x20)/' ;;
    esac
    sed "/^void func_801E37D0(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc -std=gnu17 "${common[@]}" -O2 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/object_link_retail_test.c -o "$OUT/$mutant.o"
    clang++ -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" \
        "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "OBJECT LINK mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'OBJECT LINK FAIL' "$OUT/$mutant.log"
done
echo "OBJECT LINK negative controls PASS: detach visibility joint orientation offset duplicate"
# Reintroduce the original signed-shift bug only in a scratch translation unit.
# Require a runtime sanitizer failure, not a compilation or link failure.
sed -E 's/\(long long\)CV([123])\(cv\) \* 4096/((long long)CV\1(cv) << 12)/g' \
    pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp > "$OUT/signed-shift.cpp"
g++ -std=c++17 "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all \
    -Ipc_port/extern/PsyCross/src/gte \
    -fpermissive -w -include pc_port/src/port_compat.h \
    -c "$OUT/signed-shift.cpp" -o "$OUT/signed-shift.o"
clang++ -no-pie -fsanitize=undefined -fno-sanitize-recover=all -Wl,--gc-sections \
    "$OUT/UBSan.test.o" "$OUT/UBSan.cpu.o" "$OUT/UBSan.INLINE_C.C.o" \
    "$OUT/signed-shift.o" -ldl -o "$OUT/signed-shift.test"
if "$OUT/signed-shift.test" > "$OUT/signed-shift.log" 2>&1; then
    echo "OBJECT LINK signed-shift mutant survived" >&2; exit 1
fi
rg -q 'runtime error: left shift of negative value' "$OUT/signed-shift.log"
echo "OBJECT LINK signed-shift negative control PASS"
