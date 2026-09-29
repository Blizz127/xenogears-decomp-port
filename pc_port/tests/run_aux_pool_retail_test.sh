#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/aux_pool_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x4064:0x4248]).hexdigest()=='587c6c424093b1ce08f561d48477b1176b36a89236f27f7dbd1bb8e3aecdc621'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/aux_pool_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in extra stride packet uv size order alias; do
    case "$mutant" in
        extra) owner='void func_801E011C'; expression='s/(pool + 4) + 1/(pool + 4) + 0/' ;;
        stride) owner='void func_801E011C'; expression='s/entry += 0x7C/entry += 0x78/' ;;
        packet) owner='void func_801E011C'; expression='s/j < 2/j < 1/' ;;
        uv) owner='void func_801E011C'; expression='s/0xBD/0xBC/g' ;;
        size) owner='u8\* func_801E0064'; expression='s/124u/120u/' ;;
        order) owner='void func_801E00DC'; expression='/if (entries != NULL) HeapFree(entries);/d; s/u32 emptyIndices = 0;/if (entries != NULL) HeapFree(entries); u32 emptyIndices = 0;/' ;;
        alias) owner='u8\* func_801E0064'; expression='s/memcpy(pool + 4, \&capacity, sizeof(capacity));/*(u16*)(pool + 4) = capacity;/; s/memcpy(pool + 6, \&next, sizeof(next));/*(u16*)(pool + 6) = next;/' ;;
    esac
    sed "/^$owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/aux_pool_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "AUX POOL mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'AUX POOL FAIL' "$OUT/$mutant.log"
done
echo "AUX POOL negative controls PASS: extra stride packet uv size order alias"
