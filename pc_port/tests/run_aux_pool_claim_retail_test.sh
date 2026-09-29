#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/aux_pool_claim_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x4248:0x4398]).hexdigest()=='5ff4f732dc907fb123d331f6d44b62452489a74bf5dd11f8aae6c5de175fb5b5'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/aux_pool_claim_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in sentinel mark scan packet signed index release; do
    owner='u8\* func_801E0248'
    case "$mutant" in
        sentinel) expression='s/return (u8\*)(uintptr_t)(\*(u32\*)pool + (u32)(s32)\*(s16\*)(pool + 4) \* 124u);/return NULL;/' ;;
        mark) expression='s/return entry;/\*(s16*)(entry + 0x16) = 0; return entry;/' ;;
        scan) expression='s/while (\*(s16\*)(pool + 6) < capacity)/while (0)/' ;;
        packet) expression='s/entry + 0x54/entry + 0x2C/' ;;
        signed) expression='s/(s16)semiTrans/(u16)semiTrans/g' ;;
        index) owner='s32 func_801E0354'; expression='s/difference \/ 124u/difference \/ 120u/' ;;
        release) owner='s32 func_801E0354'; expression='s/>= (s32)index/<= (s32)index/' ;;
    esac
    sed "/^$owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/aux_pool_claim_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "AUX CLAIM mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'AUX CLAIM FAIL' "$OUT/$mutant.log"
done
echo "AUX CLAIM negative controls PASS: sentinel mark scan packet signed index release"
