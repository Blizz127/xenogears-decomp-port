#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/track_pool_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x35f4:0x37a8]).hexdigest()=='81d5fc3a664b9ff11609c9dd60d64a9236cf43ab0955278c885fa9ee400a1c62'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/track_pool_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in capacity size reset order mark advance occupied; do
    case "$mutant" in
        capacity) owner='u8\* func_801DF5F4'; expression='s/(u16)count/(u16)(count + 1)/' ;;
        size) owner='u8\* func_801DF5F4'; expression='s/count \* 20u/count * 16u/' ;;
        reset) owner='void func_801DF6A8'; expression='s/entries\[0\] = 0/entries[1] = 0/' ;;
        order) owner='void func_801DF668'; expression='/if (entries != NULL) HeapFree(entries);/d; s/\*(u16\*)(pool + 4) = 0;/if (entries != NULL) HeapFree(entries); *(u16*)(pool + 4) = 0;/' ;;
        mark) owner='u8\* func_801DF6F0'; expression='s/return entry;/entry[0] = 1; return entry;/' ;;
        advance) owner='u8\* func_801DF6F0'; expression='s/(index + 1)/(index)/' ;;
        occupied) owner='u8\* func_801DF6F0'; expression='s/entry\[0\] != 0/entry[0] == 0/' ;;
    esac
    sed "/^$owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/track_pool_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "TRACK POOL mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'TRACK POOL FAIL' "$OUT/$mutant.log"
done
echo "TRACK POOL negative controls PASS: capacity size reset order mark advance occupied"
