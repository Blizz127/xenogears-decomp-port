#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/effect_update_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
b=bytearray()
with open('disc/disc1.bin','rb') as f:
    for sector in range(231361,231386):
        f.seek(sector*2352+24);b.extend(f.read(2048))
assert sha256(b).hexdigest()=='14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'
assert sha256(b[0x5258:0x565c]).hexdigest()=='bde5842f0f7b095691ed3a3be6284607384e7883073524e62cefd7bdfb2ebeff'
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/effect_update_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.cpu.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in tick active repeat cleanup source upload linked stride dirty rectangle callback; do
    owner='s32 func_801E1258'
    case "$mutant" in
        tick) expression='s/(u32)ticks + 1u/(u32)ticks/' ;;
        active) expression='s/if (\*(u16\*)(effect + 0x1A) == 0) return -1;/if (0) return -1;/' ;;
        repeat) expression='s/if (frame == \*(u16\*)(effect + 0x18)) return frame;/if (0) return frame;/' ;;
        cleanup) expression='s/func_801E165C(effect);/;/' ;;
        source) expression='s/(u16\*)(uintptr_t)\*(u32\*)(effect + 8), (u16\*)(uintptr_t)\*(u32\*)(effect + 4)/(u16*)(uintptr_t)*(u32*)(effect + 4), (u16*)(uintptr_t)*(u32*)(effect + 8)/' ;;
        upload) expression='s/if (\*(u32\*)effect == 0)/if (1)/' ;;
        linked) expression='s/u8 linkedType = linked\[0x10\];/u8 linkedType = effect[0x10];/' ;;
        stride) expression='s/\* 6u/* 8u/' ;;
        dirty) expression='s/linked\[0x11\] = 1;/linked[0x11] = 0;/' ;;
        rectangle) expression='s/if (extent\[0\] <= 0 || extent\[1\] <= 0) return frame;/if (extent[0] <= 0 || extent[1] <= 0) return frame; if (extent[0] > 3) extent[0] = 3;/' ;;
        callback) owner='static s32 FieldEffectCallback'; expression='s/__builtin_trap();/return 0;/' ;;
    esac
    sed "/^$owner(/,/^}/ { $expression; }" pc_port/src/field_object_overlay.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c pc_port/tests/effect_update_retail_test.c -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "EFFECT UPDATE mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'EFFECT UPDATE FAIL' "$OUT/$mutant.log"
done
echo "EFFECT UPDATE negative controls PASS: tick active repeat cleanup source upload linked stride dirty rectangle callback"
