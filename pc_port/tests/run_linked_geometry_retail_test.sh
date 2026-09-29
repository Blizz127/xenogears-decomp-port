#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/linked_geometry_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for s in range(231361,231386):
        f.seek(s*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x62f8:0x7438]).hexdigest()=='93902b9a120ae364d9084a84c16cc079481eb89acc84a62ec13e8f94f3e43445'
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
        -c pc_port/tests/linked_geometry_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -fpermissive -w \
        -c pc_port/src/field_object_overlay.c -o "$OUT/$opt.leaf.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.leaf.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1
    cat "$OUT/$opt.log"
done
for mutant in motion normal buffer skip color link; do
    owner='void func_801E22F8'
    case "$mutant" in
        motion) owner='static void FieldGeometryNext'; expression='s/0x100000/0x10000/' ;;
        normal) expression='s/(s32)MFC2(25) \/ 8/(s32)MFC2(25) \/ 4/' ;;
        buffer) expression='s/(u32)buffer \* 40u/(u32)buffer * 0u/' ;;
        skip) expression='s/if (CFC2(31) \& 0x40000u) continue;/if (CFC2(31) \& 0x40000u) { face += 0x58; packet += 0x58; continue; }/' ;;
        color) expression='s/orientation < 0 ? 0xC : 0xF/orientation < 0 ? 0xF : 0xC/' ;;
        link) expression='s/(\*bucket \& 0xFF000000u)/(0u)/' ;;
    esac
    sed "/^$owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" "${cflags[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" \
        "$OUT/$mutant.o" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "LINKED GEOMETRY mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'LINKED GEOMETRY FAIL' "$OUT/$mutant.log"
done
echo "LINKED GEOMETRY negative controls PASS: motion normal buffer skip color link"
sed '/^int GTE_RotTransPers(/,/^}/s/(long long)C2_TRZ \* 4096/((long long)C2_TRZ << 12)/' \
    pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp > "$OUT/projection-shift.cpp"
g++ -std=c++17 "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all -fpermissive -w \
    -Ipc_port/extern/PsyCross/src/gte -include pc_port/src/port_compat.h \
    -c "$OUT/projection-shift.cpp" -o "$OUT/projection-shift.o"
clang++ -no-pie -fsanitize=undefined -fno-sanitize-recover=all -Wl,--gc-sections \
    "$OUT/UBSan.test.o" "$OUT/UBSan.cpu.o" "$OUT/UBSan.leaf.o" \
    "$OUT/UBSan.INLINE_C.C.o" "$OUT/projection-shift.o" -o "$OUT/projection-shift.test"
if "$OUT/projection-shift.test" > "$OUT/projection-shift.log" 2>&1; then
    echo "LINKED GEOMETRY projection shift mutant survived" >&2; exit 1
fi
rg -q 'runtime error: left shift of negative value' "$OUT/projection-shift.log"
echo "LINKED GEOMETRY projection negative control PASS: original signed shift rejected"
