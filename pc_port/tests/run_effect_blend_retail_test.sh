#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/effect_blend_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x5708:0x5880]).hexdigest()=='930c868225e060b0e36f6166c70e8fe3caaac7e9ce348ea7b573ab2c5bcafeaf'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/effect_blend_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in stride signed rounding source factor base; do
    owner='void func_801E1708'
    case "$mutant" in
        stride) expression='s/\* 6u/* 8u/' ;;
        signed) expression='s/(s32)\*source/(s32)(s16)*source/' ;;
        rounding) expression='s/product \/ 32/product >> 5/' ;;
        source) expression='s/++source;/source += 2;/' ;;
        factor) expression='s/(s16)factor/(u16)factor/' ;;
        base) owner='void func_801E17B8'; expression='s/\*base + product \/ 32/product \/ 32/' ;;
    esac
    sed "/^$owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/effect_blend_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "EFFECT BLEND mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'EFFECT BLEND FAIL' "$OUT/$mutant.log"
done
echo "EFFECT BLEND negative controls PASS: stride signed rounding source factor base"
