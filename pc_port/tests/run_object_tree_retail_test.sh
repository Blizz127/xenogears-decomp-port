#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/object_tree_retail_test
scaled=()
if [ "${1:-}" = --scaled ]; then
    OUT=pc_port/build_native/object_scaled_tree_retail_test
    scaled=(-DOBJECT_TREE_SCALED)
fi
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x5c0:0x848]).hexdigest()=='36086b6f0d87dd02bb67c9f407c8a0cc29c4dc43edd036bdf2e5e474b37dce44'
assert sha256(b[0x848:0xc34]).hexdigest()=='5ccf892d0ec38181361a279503da3497bf101eab4cfaef848420fd223df3813e'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    "${scaled[@]}"
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/object_tree_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
mutants=(scale parent dirty rotation clear translation copy)
if [ "${1:-}" = --scaled ]; then mutants+=(inherit reciprocal clear_rotation zero_fallback); fi
for mutant in "${mutants[@]}"; do
    case "$mutant" in
        scale) expression='s/product >> 12/product >> 16/' ;;
        parent) expression='s/(MATRIX\*)(parent + 0x2C)/(MATRIX*)(root + 0x2C)/' ;;
        dirty) expression='s/parent\[4\] == 1/parent[4] != 0/' ;;
        rotation) expression='s/RotMatrixYXZ(/RotMatrix(/g' ;;
        clear) expression='s/root\[i \* OVLY_NODE_STRIDE + 4\] = 0/root[i * OVLY_NODE_STRIDE + 4] = 1/' ;;
        translation) expression='s/root + 0x40, root + 0x5C/root + 0x40, root + 0x4C/' ;;
        copy) expression='s/memcpy(node + 0x2C, node + 0x0C, sizeof(MATRIX))/memcpy(node + 0x2C, root + 0x0C, sizeof(MATRIX))/' ;;
        inherit) expression='s/parent\[5\] == 1/0/' ;;
        reciprocal) expression='s/0x1000000 \/ divisor/0x1000 \/ divisor/' ;;
        clear_rotation) expression='s/root\[i \* OVLY_NODE_STRIDE + 5\] = 0/root[i * OVLY_NODE_STRIDE + 5] = 1/' ;;
        zero_fallback) expression='s/__builtin_trap()/return 0/' ;;
    esac
    owner=func_801DC5C0
    if [ "${1:-}" = --scaled ]; then owner=func_801DC848; fi
    sed "/^s32 $owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/object_tree_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "OBJECT TREE mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'OBJECT TREE FAIL' "$OUT/$mutant.log"
done
echo "OBJECT TREE negative controls PASS: ${mutants[*]}"
