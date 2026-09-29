#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/linked_geometry_cleanup_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x7438:0x74bc]).hexdigest()=='40f036f5def72dd96f6fdf1357db7bc47c35e26655b956a79bdbba78fd3794a5'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/linked_geometry_cleanup_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in active table optional clear; do
    case "$mutant" in
        active) expression='s/if (\*(u32\*)(owner + 0x14) == 0) return;/if (0) return;/' ;;
        table) expression='s/HeapFree((void\*)(uintptr_t)\*(u32\*)(uintptr_t)\*(u32\*)(owner + 0x1C));/HeapFree((void*)(uintptr_t)*(u32*)(owner + 0x1C));/' ;;
        optional) expression='s/if (\*(u32\*)(owner + 0x18) != 0)/if (1)/' ;;
        clear) expression='s/\*(u32\*)(owner + 0x14) = 0;/memset(owner, 0, 0x24);/' ;;
    esac
    sed "/^void func_801E3438(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/linked_geometry_cleanup_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "GEOMETRY CLEANUP mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'GEOMETRY CLEANUP FAIL' "$OUT/$mutant.log"
done
echo "GEOMETRY CLEANUP negative controls PASS: active table optional clear"
