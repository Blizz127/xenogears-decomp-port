#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/node_cleanup_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0xd8c:0xe18]).hexdigest()=='4b9ad7ffc3d4742466f4ef70efbc8e210d78f0b8ce6dcec9ece683933a4bfcba'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/node_cleanup_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
    "$OUT/$opt.test" --adapter
done
for mutant in limit null_second count order stride; do
    case "$mutant" in
        limit) expression='s/i < \*(u16\*)(root + 0x0A)/i < *(u16*)(root + 0x0A) \&\& i < 1024/' ;;
        null_second) expression='s/if (packet != NULL) {/if (packet == NULL) *(u32*)(node + 0x6C) = 0; if (packet != NULL) {/' ;;
        count) expression='/\*(u16\*)(root + 0x0A) = 0;/d' ;;
        order) expression='/HeapFree(root);/d; s/\*(u16\*)(root + 0x0A) = 0;/HeapFree(root); *(u16*)(root + 0x0A) = 0;/' ;;
        stride) expression='s/node += 0x7C/node += 0x78/' ;;
    esac
    sed "/^void func_801DCD8C(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/node_cleanup_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "NODE CLEANUP mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'NODE CLEANUP FAIL' "$OUT/$mutant.log"
done
echo "NODE CLEANUP negative controls PASS: limit null_second count order stride"
