#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/model_registry_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x22c:0x2d0]).hexdigest()=='ed37d38e4361912726b17fd6aeb8902f96e13650ba9caf105aa5347b7aa4e971'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/model_registry_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in offset stride size count zero relocation; do
    case "$mutant" in
        offset) expression='s/offset = 0x10/offset = 0x14/' ;;
        stride) expression='s/offset += 0x38/offset += 0x34/' ;;
        size) expression='s/HeapAlloc(count \* 4u/HeapAlloc(count * 8u/' ;;
        count) expression='s/memcpy(out + 4, \&count, sizeof(count));/count = 0; memcpy(out + 4, \&count, sizeof(count));/' ;;
        zero) expression='s/u32 pointers =/if (count == 0) return out; u32 pointers =/' ;;
        relocation) expression='s/(u32)func_8002C3E8(model)/0/' ;;
    esac
    sed "/^u8\* func_801DC22C(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/model_registry_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "MODEL REGISTRY mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'MODEL REGISTRY FAIL' "$OUT/$mutant.log"
done
echo "MODEL REGISTRY negative controls PASS: offset stride size count zero relocation"
