#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_dispatch_93_retail_test
mkdir -p "$OUT"
python3 - <<'CHECK'
from hashlib import sha256
from pathlib import Path
assert sha256(Path('disc/SLUS_006.64').read_bytes()).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
CHECK
for opt in O0 O2 UBSan; do
 flags=(-"$opt"); if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc -std=gnu17 -fpermissive "${flags[@]}" -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DUSE_EXTENDED_PRIM_POINTERS=0 -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c src/slus_006.64/system/animation_scripts.c -o "$OUT/$opt.body.o"
 # Borrow dependency guards: any unexpected helper call aborts the fixture.
 gcc -std=gnu17 "${flags[@]}" -Dmain=noop_main -Dfunc_8001D4E8=GuardDirection93 -DHeapAlloc=GuardHeap93 -Dfunc_8001D2B0=GuardFrame93 -Dfunc_8001EE68=GuardFormat93 -fno-pie -Ipc_port/src -c pc_port/tests/sprite_dispatch_noop_retail_test.c -o "$OUT/$opt.guards.o"
 for unit in rendering temp1;do
  gcc -std=gnu17 -fpermissive "${flags[@]}" -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DUSE_EXTENDED_PRIM_POINTERS=0 -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "src/slus_006.64/system/$unit.c" -o "$OUT/$opt.$unit.o"
  objcopy --weaken "$OUT/$opt.$unit.o"
 done
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${flags[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_93_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" "$OUT/$opt.guards.o" "$OUT/$opt.rendering.o" "$OUT/$opt.temp1.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
echo 'SPRITE 93 O0/O2/UBSAN PASS'

python3 - <<'PY'
from pathlib import Path
p=Path('src/slus_006.64/system/animation_scripts.c');s=p.read_text();a=s.index('    case 0x93: {');b=s.index('    case 0xBD:',a);body=s[a:b]
for name,old,new in [('mode-gate','(AnimationRead32(p + 0x3C) & 3u) == 0','(AnimationRead32(p + 0x3C) & 3u) == 7'),('type-bits','| 0x1C000u','| 0x1A000u'),('copy-count','i < 8','i < 7'),('signed-frame','(s16)AnimationRead16(p + 0x34)','(s16)(AnimationRead16(p + 0x34) + 1)')]:
 assert body.count(old)==1,name
 Path('pc_port/build_native/sprite_dispatch_93_retail_test',name+'.c').write_text(s[:a]+body.replace(old,new)+s[b:])
s=Path('src/slus_006.64/system/rendering.c').read_text();a=s.index('void func_8001D4E8(');b=s.index('extern void func_800251C8',a);body=s[a:b]
for name,old,new in [('heap-size','HeapAlloc(0x40, 0)','HeapAlloc(0x38, 0)'),('model-reload','        void* directions = HeapAlloc(0x40, 0);\n        /* Retail reloads the model after HeapAlloc returns. */\n        u8* model = (u8*)(uintptr_t)*(u32*)(sprite + 0x20);','        u8* model = (u8*)(uintptr_t)*(u32*)(sprite + 0x20);\n        void* directions = HeapAlloc(0x40, 0);')]:
 assert body.count(old)==1,name
 Path('pc_port/build_native/sprite_dispatch_93_retail_test',name+'.c').write_text(s[:a]+body.replace(old,new)+s[b:])
PY
for mutant in mode-gate type-bits copy-count signed-frame heap-size model-reload;do
 gcc -std=gnu17 -fpermissive -O2 -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DUSE_EXTENDED_PRIM_POINTERS=0 -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 if [[ "$mutant" == heap-size || "$mutant" == model-reload ]];then
  objcopy --weaken "$OUT/$mutant.o"
  objects=("$OUT/O2.body.o" "$OUT/$mutant.o")
 else
  objects=("$OUT/$mutant.o" "$OUT/O2.rendering.o")
 fi
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie -O2 -Wl,--gc-sections pc_port/tests/sprite_dispatch_93_retail_test.c pc_port/src/battle_mips_adapter.c "${objects[@]}" "$OUT/O2.temp1.o" "$OUT/O2.guards.o" -o "$OUT/$mutant.test"
 rc=0
 "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! rg -q 'SPRITE93 FAIL case=' "$OUT/$mutant.log";then cat "$OUT/$mutant.log";exit 1;fi
 echo "SPRITE93 MUTANT REJECTED $mutant"
done
