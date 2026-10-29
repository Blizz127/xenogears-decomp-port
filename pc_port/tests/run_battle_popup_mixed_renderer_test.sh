#!/usr/bin/env bash
set -euo pipefail
if [[ ${XENO_POPUP_MIXED_BOUNDED:-0} != 1 ]]; then
 exec timeout -k 5s 90s env XENO_POPUP_MIXED_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
out=$(mktemp -d "${TMPDIR:-/tmp}/xeno-popup-mixed.XXXXXX")
trap 'rm -rf "$out"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/battle.bin | cut -d' ' -f1) == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
python3 - "$out" <<'PY'
from pathlib import Path
import sys
out=Path(sys.argv[1]);inc=Path('src/battle/anim_render_index15.inc').read_text()
def function(sig):
 start=inc.index(sig);brace=inc.index('{',start);depth=0
 for i in range(brace,len(inc)):
  depth+=(inc[i]=='{')-(inc[i]=='}')
  if depth==0:return inc[start:i+1]
 raise ValueError(sig)
prologue='''#include <stdint.h>
#include <stddef.h>
#include "psx_memory.h"
#include "guest_prim_link.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include <psx/inline_c.h>
#include <psx/gtereg.h>
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16; typedef uint32_t u32; typedef int32_t s32;
extern s32 D_80050100; extern u32 g_GfxCurWorkBuffer; extern u32 g_GfxCurWorkBufferEnd;
#define ANIM_RENDER_TPAGE_PACKET 0x800C3BF8u
void func_800B1F0C(u32*);
'''
sigs=['static u8* BattleAnimRenderAddress(','static void BattleAnimRenderTriangle(','static void BattleAnimRenderQuad(','void func_800B1F0C(','void func_800B1F6C(']
(out/'renderer.c').write_text(prologue+'\n'.join(function(x) for x in sigs))
PY
common=(-fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -I"$out" -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
 g++ -std=c++17 "${common[@]}" -O0 -fpermissive -w -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$out/${source##*/}.o"
done
for pair in "test:pc_port/tests/anim_render_renderer_retail_test.c" "renderer:$out/renderer.c" "cpu:pc_port/src/battle_mips_adapter.c" "link:pc_port/src/guest_prim_link.c"; do
 name=${pair%%:*};source=${pair#*:};gcc -std=gnu17 "${common[@]}" -O0 -w -c "$source" -o "$out/$name.o"
done
g++ -no-pie -O0 -Wl,--gc-sections "$out/test.o" "$out/renderer.o" "$out/cpu.o" "$out/link.o" "$out/LIBGTE.C.o" "$out/INLINE_C.C.o" "$out/PsyX_GTE.cpp.o" -lstdc++ -lm -o "$out/test"
env -u XENO_DISABLE_CPU_INTERPRETER XENO_POPUP_MIXED_ONLY=1 "$out/test"
