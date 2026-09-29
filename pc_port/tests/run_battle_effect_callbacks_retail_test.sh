#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/battle_effect_callbacks.XXXXXXXX)
echo "Artifacts: $TEST_OUT"
echo '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291  disc/battle.bin' | sha256sum -c -
echo 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119  disc/SLUS_006.64' | sha256sum -c -
bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --out "$TEST_OUT/battle_bridge_map.inc"
rg -q '0x8003f8b0u, 0u, 1, "rcos"' "$TEST_OUT/battle_bridge_map.inc"
rg -q '0x8003f8ccu, 0u, 1, "rsin"' "$TEST_OUT/battle_bridge_map.inc"

common=(-fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
failed=0
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h \
            -c "pc_port/extern/PsyCross/src/$source" -o "$TEST_OUT/$opt.$name.o"
    done
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$TEST_OUT" \
        -Wall -Wextra -Werror -c pc_port/tests/battle_effect_callbacks_retail_test.c -o "$TEST_OUT/$opt.test.o"
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$TEST_OUT/$opt.cpu.o"
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h \
        -DXENO_BATTLE_OVERLAY_HOST_BODIES -Ipc_port/include_shim -Iinclude -Ipc_port/src -c src/battle/main68.c -o "$TEST_OUT/$opt.effects.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections \
        -Wl,--export-dynamic-symbol=rsin -Wl,--export-dynamic-symbol=rcos \
        -Wl,--export-dynamic-symbol=func_800A3490 -Wl,--export-dynamic-symbol=func_800A3514 \
        -Wl,--export-dynamic-symbol=func_800A3578 -Wl,--export-dynamic-symbol=func_800A35C8 \
        -Wl,--section-start=.trig_psx_ram=0x10000020 -Wl,--section-start=.trig_scratch=0x10300040 \
        "$TEST_OUT/$opt.effects.o" "$TEST_OUT/$opt.test.o" "$TEST_OUT/$opt.cpu.o" "$TEST_OUT/$opt.LIBGTE.C.o" \
        "$TEST_OUT/$opt.INLINE_C.C.o" "$TEST_OUT/$opt.PsyX_GTE.cpp.o" -ldl -o "$TEST_OUT/$opt.test"
    if "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1; then
        cat "$TEST_OUT/$opt.log"
    else
        cat "$TEST_OUT/$opt.log" >&2
        failed=1
    fi
done
test "$failed" = 0

# Negative controls: distinguish the retail cap, signed wrap-before-compare,
# and symbol-map trig convention from superficially plausible alternatives.
for mutant in cap wrap trig; do
    python3 - "$mutant" "$TEST_OUT" <<'PYM'
from pathlib import Path
import sys
s=Path('src/battle/main68.c').read_text()
a,b={'cap':('value < 0x21','value < 0x20'),
     'wrap':('s16 temp_v0;','s32 temp_v0;'),
     'trig':('rsin(arg0)','rcos(arg0)')}[sys.argv[1]]
assert s.count(a)==1
(Path(sys.argv[2])/(sys.argv[1]+'.c')).write_text(s.replace(a,b))
PYM
    gcc -std=gnu17 "${common[@]}" -O2 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -DXENO_BATTLE_OVERLAY_HOST_BODIES \
        -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        -c "$TEST_OUT/$mutant.c" -o "$TEST_OUT/$mutant.o"
    clang++ -no-pie -Wl,--gc-sections \
        -Wl,--export-dynamic-symbol=rsin -Wl,--export-dynamic-symbol=rcos \
        -Wl,--export-dynamic-symbol=func_800A3490 -Wl,--export-dynamic-symbol=func_800A3514 \
        -Wl,--export-dynamic-symbol=func_800A3578 -Wl,--export-dynamic-symbol=func_800A35C8 \
        -Wl,--section-start=.trig_psx_ram=0x10000020 -Wl,--section-start=.trig_scratch=0x10300040 \
        "$TEST_OUT/$mutant.o" "$TEST_OUT/O2.test.o" "$TEST_OUT/O2.cpu.o" \
        "$TEST_OUT/O2.LIBGTE.C.o" "$TEST_OUT/O2.INLINE_C.C.o" "$TEST_OUT/O2.PsyX_GTE.cpp.o" \
        -ldl -o "$TEST_OUT/$mutant.test"
    if "$TEST_OUT/$mutant.test" > "$TEST_OUT/$mutant.log" 2>&1; then
        echo "ERROR: mutant survived: $mutant" >&2; exit 1
    fi
    grep -q '^effect ' "$TEST_OUT/$mutant.log"
    echo "Negative control rejected: $mutant"
done
