#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_EFFECT_RESET_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-effect-reset-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-symbolize=0}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE EFFECT RESET artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1]);module=pathlib.Path('scratchpad/native-module-20261017/module-test-2/module.bin')
identities={str(module):'4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885',
 'disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291',
 'disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'}
for p,d in identities.items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
(out/'module.bin').write_bytes(module.read_bytes())
source=pathlib.Path('src/battle/main41.c').read_text()
a=source.index('u16 func_80089C08(u8 index) {');b=source.index('\n}',a)+2;mask=source[a:b]
(out/'mask.c').write_text('#include <stdint.h>\n#include "psx_memory.h"\ntypedef uint16_t u16;typedef uint8_t u8;\n#include "battle_overlay_guest_ram.h"\n'+mask+'\n')
memory=pathlib.Path('src/slus_006.64/system/memory.c').read_text()
bodies=[]
for signature in ['void* HeapAlloc(', 'void HeapConsolidate(', 'void HeapChangeCurrentUser(']:
 a=memory.index(signature);b=memory.index('\n}',a)+2;bodies.append(memory[a:b])
(out/'native-heap.inc').write_text('\n\n'.join(bodies)+'\n')
(out/'heap-support.c').write_text(r'''#include "common.h"
#include "system/memory.h"
#include "psx_memory.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
void *g_Heap;
u32 g_HeapNeedsConsolidation,g_HeapIsErrorHandlerOff,g_HeapLastAllocSize,g_HeapLastAllocSrcAddr;
u16 g_HeapCurUser,g_HeapCurContentType;
char **g_HeapUserContentNames[HEAP_NUM_USERS];
void MainLoop(int error) { fprintf(stderr,"unexpected heap error %d\n",error);abort(); }
#include "native-heap.inc"
void allocation_fixture_reset(unsigned variant)
{
 u8 *arena=PSX_ADDR(0x80100000u);HeapBlock *first=(HeapBlock *)arena;
 HeapBlock *end=(HeapBlock *)(arena+0x3ff0);
 memset(first,0,sizeof(*first));memset(end,0,sizeof(*end));end->userTag=HEAP_USER_END;
 first->pNext=(mem_addr)(uintptr_t)(end+1);first->contentTag=HEAP_CONTENT_FREE;
 if(variant&24u) {
  HeapBlock *mid=(HeapBlock *)(arena+0x100);memset(mid,0,sizeof(*mid));
  mid->pNext=first->pNext;mid->contentTag=HEAP_CONTENT_FREE;
  first->pNext=(mem_addr)(uintptr_t)(mid+1);if(variant&16u) first->userTag=HEAP_USER_KAZM;
 }
 g_Heap=first+1;g_HeapCurUser=7;g_HeapCurContentType=HEAP_CONTENT_MODEL_DATA;
 g_HeapNeedsConsolidation=(variant&4u)!=0;g_HeapIsErrorHandlerOff=1;
 g_HeapLastAllocSize=0xabcdef;g_HeapLastAllocSrcAddr=0x12345678;
 memset(g_HeapUserContentNames,0,sizeof(g_HeapUserContentNames));
}
void allocation_fixture_state(uint64_t *out)
{
 memset(out,0,16*sizeof(*out));out[0]=g_HeapCurUser;out[1]=g_HeapCurContentType;
 out[2]=g_HeapNeedsConsolidation;out[3]=g_HeapIsErrorHandlerOff;
 out[4]=g_HeapLastAllocSize;out[5]=g_HeapLastAllocSrcAddr;
 out[6]=(uintptr_t)g_Heap;out[7]=(uintptr_t)g_HeapUserContentNames[4];
}
''')
paths=list(map(str,pathlib.Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
 'pc_port/src/controller_vblank_service.c','pc_port/src/battle_overlay_guest_ram.h','src/battle/main41.c',
 'src/slus_006.64/system/temp1f.c','pc_port/extern/PsyCross/src/psx/LIBGPU.C',
 'pc_port/src/retail_data.h','src/battle/main4.c','src/battle/main5.c',
 'pc_port/src/retail_leaf_adapters.c','pc_port/src/psyq_compat_leaf.inc','pc_port/extern/PsyCross/include/psx/libgpu.h',
 'pc_port/tests/battle_module_stage1_host_test.c','pc_port/tests/battle_module_stage3_host_test.c',
 'src/slus_006.64/system/memory.c','include/system/memory.h','pc_port/tests/battle_effect_reset_host_test.c','pc_port/tests/run_battle_effect_reset_host_test.sh','pc_port/tests/battle_effect_reset_host_test.c','pc_port/tests/run_battle_effect_reset_host_test.sh']
(out/'provenance.json').write_text(json.dumps(dict(scope='Six connected original effect-pool/reset bodies, actual native heap and SDK integration, plus explicitly modeled boundary cases. Original heap policy and full battle rendering not accepted.',
 private_inputs=identities,source_pins={p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in paths}),indent=2)+'\n')
for name in paths:
 target=out/'source-snapshot'/name;target.parent.mkdir(parents=True,exist_ok=True)
 target.write_bytes(pathlib.Path(name).read_bytes())

PYCODE
python3 tools/scripts/gen_battle_bridge_map.py --symbols config/symbol_addrs.slus_006.64.txt \
 --symbols linker/undefined_funcs_auto.battle.txt --symbols linker/undefined_syms_auto.battle.txt \
 --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"
common=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -include assert.h -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
sdk_common=(-std=c++17 -fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
 -fpermissive -w -include pc_port/src/port_compat.h -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"
for mode in ${BATTLE_EFFECT_RESET_TEST_MODES:-O0 O2 UBSan};do
 compiler=gcc;flags=(-"$mode");atlas_flags=(-fpermissive)
 if [ "$mode" = UBSan ];then compiler=clang;flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);atlas_flags=();fi
 clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
 gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c "$OUT/heap-support.c" -o "$OUT/$mode.heap.o"
 for spec in test:pc_port/tests/battle_effect_reset_host_test.c cpu:pc_port/src/battle_mips_adapter.c \
             vblank:pc_port/src/controller_vblank_service.c mask:"$OUT/mask.c";do
  "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "${spec#*:}" -o "$OUT/$mode.${spec%%:*}.o"
 done
 g++ "${sdk_common[@]}" "${sdl_flags[@]}" "${flags[@]}" -c pc_port/extern/PsyCross/src/psx/LIBGPU.C -o "$OUT/$mode.gpu.o"
 clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.test.o" \
  "$OUT/$mode.cpu.o" "$OUT/$mode.heap.o" "$OUT/$mode.vblank.o" "$OUT/$mode.mask.o" "$OUT/$mode.gpu.o" -ldl -lcrypto -o "$OUT/$mode.test"
done
for mode in ${BATTLE_EFFECT_RESET_TEST_MODES:-O0 O2 UBSan};do
 args=();if [ "${BATTLE_EFFECT_RESET_CONTROLS_ONLY:-0}" = 1 ];then args=(--controls-only);fi
 if ! "$OUT/$mode.test" "${args[@]}" > "$OUT/$mode.test.log" 2>&1;then cat "$OUT/$mode.test.log" >&2;exit 1;fi
 rg '^BATTLE EFFECT RESET (PASS|completed) ' "$OUT/$mode.test.log"
done
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE EFFECT RESET source pins unchanged')
PYCODE
