#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/node_constructor_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x2d0:0x5c0]).hexdigest()=='d10a984d37284d898fcea03ff7360ef9d0cb0a0e0e41046775aa7020c1681e6a'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/node_constructor_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in terminator parent mode setup tracks matrices cleanup lookup mesh; do
    owner='u8\* func_801DC2D0'
    case "$mutant" in
        terminator) expression='s/ || list\[records \* 2\] == 0xFFFF//' ;;
        parent) expression='s/(u32)parentIndex + 1u/(u32)parentIndex + 0u/' ;;
        mode) expression='s/, mode);/, 0);/' ;;
        setup) expression='s/if (setup != 0)/if (mode != 0)/' ;;
        tracks) expression='s/\*(u32\*)(node + 0x70) =/\*(u32\*)(node + 0x70) = 0x1234;/' ;;
        matrices) expression='s/root\[4\] =/memset(root + 0xC, 0, 0x40); root[4] =/' ;;
        cleanup) expression='/func_801DCD8C(root);/d' ;;
        lookup) owner='static u8\* OvlyNodeModel'; expression='s/return index ==/return (u8*)(uintptr_t)*(u32*)(node + 0x70); return index ==/' ;;
        mesh) expression='s/u8\* model = (u8\*)(uintptr_t)pointers\[modelIndex\];/u8* model = (u8*)(uintptr_t)pointers[modelIndex]; *(u16*)(model + 6) = 1;/' ;;
    esac
    sed "/^$owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/node_constructor_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "NODE CONSTRUCTOR mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'NODE CONSTRUCTOR FAIL' "$OUT/$mutant.log"
done
echo "NODE CONSTRUCTOR negative controls PASS: terminator parent mode setup tracks matrices cleanup lookup mesh"
