#!/usr/bin/env bash
# Differential regression test for func_801DCEC8 (archive 6B9 object draw):
# retail bytes on the MIPS interpreter vs the native body, at -O0 / -O2 /
# UBSan, then deliberately mutated native bodies that MUST be rejected at
# runtime (a mutant that fails to compile is reported as such, not as a
# rejection).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/object_draw_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
print('archive 6B9 payload sha256 ok; func_801DCEC8 slice', sha256(b[0xec8:0x1bf8]).hexdigest())
PY
common=(-fno-pie -ffunction-sections -fdata-sections -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
build_test() {
    local tag=$1 src=$2; shift 2
    gcc -std=gnu17 "${common[@]}" "$@" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$src\"" \
        -c pc_port/tests/object_draw_retail_test.c -o "$OUT/$tag.test.o"
}
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w -include pc_port/src/port_compat.h \
            -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.${source##*/}.o"
    done
    build_test "$opt" "$(pwd)/pc_port/src/field_object_overlay.c" "${flags[@]}"
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -ldl -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
# Negative controls: each mutant is applied inside func_801DCEC8 only, must
# compile, and must be rejected by the differential run.
for mutant in nolight localmatrix novisibility variant shade billboard rootlight; do
    case "$mutant" in
        nolight)      expression='s/SetLightMatrix(temp);/(void)0;/' ;;
        localmatrix)  expression='s/CompMatrix(rootView, (MATRIX\*)(node + 0x2C), temp);/CompMatrix(rootView, (MATRIX*)(node + 0x0C), temp);/' ;;
        novisibility) expression='s/if (mesh == 0xFFFF || node\[7\] == 0) {/if (mesh == 0xFFFF) {/' ;;
        variant)      expression='s/(void\*)ot, drawArg);/(void*)ot, 0);/' ;;
        shade)        expression='s/obj\[0x39\] = (u8)(0x6B - (otIndex >> 3));/obj[0x39] = (u8)(0x6B - (otIndex >> 2));/' ;;
        billboard)    expression='s/temp->m\[0\]\[1\] = rootView->m\[0\]\[1\];/temp->m[0][1] = rootView->m[1][0];/' ;;
        rootlight)    expression='s/MulMatrix0(light, (MATRIX\*)(root + 0x2C), local);/MulMatrix0(light, (MATRIX*)(root + 0x0C), local);/' ;;
    esac
    sed "/^void func_801DCEC8(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    if cmp -s "$OUT/$mutant.c" pc_port/src/field_object_overlay.c; then
        echo "OBJECT DRAW mutant did not apply: $mutant" >&2; exit 1
    fi
    if ! build_test "$mutant" "$(pwd)/$OUT/$mutant.c" -O2 2> "$OUT/$mutant.cc.log"; then
        echo "OBJECT DRAW mutant failed to COMPILE (not a rejection): $mutant" >&2
        cat "$OUT/$mutant.cc.log" >&2; exit 1
    fi
    clang++ -no-pie -Wl,--gc-sections "$OUT/$mutant.test.o" "$OUT/O2.cpu.o" \
        "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "OBJECT DRAW mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'OBJECT DRAW FAIL' "$OUT/$mutant.log"
done
echo "OBJECT DRAW negative controls PASS: nolight localmatrix novisibility variant shade billboard rootlight"
