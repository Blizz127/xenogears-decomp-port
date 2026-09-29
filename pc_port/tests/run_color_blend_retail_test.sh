#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/color_blend_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
b=Path('disc/SLUS_006.64').read_bytes()
assert sha256(b[0x177e8:0x1789c]).hexdigest()=='df91dc9c5881d3488381d84c56ca367b3f5a46aa7c77877ae6e87450814eabaa'
PY
common=(-fno-pie -ffunction-sections -fdata-sections -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
cflags=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all -DCOLOR_BLEND_UBSAN); fi
    for source in psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/$opt.$name.o"
    done
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/color_blend_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    gcc "${common[@]}" "${cflags[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$opt.leaf.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" \
        "$OUT/$opt.leaf.o" "$OUT/$opt.INLINE_C.C.o" "$OUT/$opt.PsyX_GTE.cpp.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1
    if [ "$opt" = UBSan ]; then rg -q 'runtime error: load of null pointer' "$OUT/$opt.log"; fi
    cat "$OUT/$opt.log"
done
for mutant in clamp direction mask command alpha trailing; do
    case "$mutant" in
        clamp) expression='/if (factor > 32) factor = 32;/d' ;;
        direction) expression='s/(inputB \& 0x001Fu) - red/red - (inputB \& 0x001Fu)/' ;;
        mask) expression='s/MFC2(10) \& 0x03E0/MFC2(10) \& 0x001F/' ;;
        command) expression='s/0x0198003D/0x0190003D/' ;;
        alpha) expression='s/(red | green | blue)/(red | green | blue | (inputA \& 0x8000))/' ;;
        trailing) expression='s/u32 remaining = (u32)count;/u32 remaining = (u32)count; if (remaining == 0) return;/' ;;
    esac
    sed "/^void func_80026FE8(/,/^}/ { $expression; }" pc_port/src/retail_leaf_adapters.c > "$OUT/$mutant.c"
    gcc "${common[@]}" "${cflags[@]}" -O2 -Wall -Wextra -Werror -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang++ -no-pie -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.cpu.o" \
        "$OUT/$mutant.o" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "COLOR BLEND mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'COLOR BLEND FAIL' "$OUT/$mutant.log"
done
echo "COLOR BLEND negative controls PASS: clamp direction mask command alpha trailing"
