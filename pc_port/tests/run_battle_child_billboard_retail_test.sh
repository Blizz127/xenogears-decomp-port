#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=${BATTLE_CHILD_BILLBOARD_OUT:-$(mktemp -d pc_port/build_native/battle_child_billboard_retail_test.XXXXXXXX)}
echo "BATTLE CHILD BILLBOARD artifacts: $OUT"
python3 tools/scripts/gen_battle_bridge_map.py --symbols config/symbol_addrs.slus_006.64.txt --symbols linker/undefined_funcs_auto.battle.txt --symbols linker/undefined_syms_auto.battle.txt --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
out=Path(sys.argv[1]); temp=Path('src/slus_006.64/system/temp1e.c').read_text(); overrides=Path('pc_port/src/game_overrides.c').read_text()
def fn(src,sig):
 # Match the definition, not a forward declaration: the signature must be
 # followed by its opening '{' before any terminating ';'.
 start=0
 while True:
  s=src.index(sig,start)
  brace=src.find('{',s); semi=src.find(';',s)
  if brace!=-1 and (semi==-1 or brace<semi):
   depth=0
   for i in range(brace,len(src)):
    depth += (src[i]=='{')-(src[i]=='}')
    if depth==0:return src[s:i+1]
  start=s+len(sig)
table=overrides[overrides.index('static void (*const D_8004FD40'):overrides.index('\n};',overrides.index('static void (*const D_8004FD40'))+3]
p='''#include <stdint.h>\n#include <stdio.h>\ntypedef uint8_t u8;typedef uint16_t u16;typedef int16_t s16;typedef uint32_t u32;typedef int32_t s32;typedef unsigned long u_long;\ntypedef struct {s16 vx,vy,vz,pad;} SVECTOR;typedef struct {s32 vx,vy,vz,pad;} VECTOR;typedef struct {s16 m[3][3];s32 t[3];} MATRIX;\n#include "psx_memory.h"\nextern u8 D_800C3664;extern s32 D_80050100;extern MATRIX D_8004FBB8;extern u_long *g_GfxCurOT;\nextern void SetRotMatrix(MATRIX*);extern void SetTransMatrix(MATRIX*);extern int RotTransPers(SVECTOR*,int*,long*,long*);extern MATRIX* TransMatrix(MATRIX*,VECTOR*);extern void func_80022038(void*);extern void func_8001E3D8(void*,void*);extern void func_8001E298(void*,void*);\nextern void func_80025710(void*),func_80025718(void*),func_8002541C(void*),func_80025544(void*),func_800257F0(void*);extern void WorkListSetTaskCallback(void*,void(*)(void*));\n'''
p+=fn(temp,'static u8* SpriteRenderAddress(')+'\n'+fn(temp,'void func_80025258(')+'\n'+table+'\n'+fn(overrides,'void func_80025224(')+'\n'
(out/'production.c').write_text(p)
(out/'mutant-packed64.c').write_text(p.replace('SpriteRenderAddress(*(u32*)(pEntry + 0x04))','*(u8**)(pEntry + 0x04)'))
(out/'mutant-gte-reversed.c').write_text(p.replace('if (flg & 0x8000)', 'if (!(flg & 0x8000))'))
(out/'mutant-bit25-reversed.c').write_text(p.replace('? 0xFFF : *(s16*)(pSprite + 0x30)', '? *(s16*)(pSprite + 0x30) : 0xFFF'))
(out/'mutant-stale-flags.c').write_text(p.replace('(*(u32*)(pSprite + 0x3C) & 0x02000000u)', '(flags3C & 0x02000000u)'))
(out/'mutant-no-normal.c').write_text(p.replace('if ((u32)depth - 1u < 0xFFFu) {\n            func_8001E298', 'if (((flags3C >> 29) & 1) && (u32)depth - 1u < 0xFFFu) {\n            func_8001E298'))
(out/'mutant-ot-stride.c').write_text(p.replace('((u32)depth << 2)', '((u32)depth << 5)'))
q=p
for line in ['(void (*)(void*))func_80025258, /* 0: billboard */','(void (*)(void*))func_80025258, /* 5: billboard */','(void (*)(void*))func_80025258, /* 6: billboard */','(void (*)(void*))func_80025258, /* 14: billboard */']:
 q=q.replace(line,'NULL, /* mutant missing billboard */')
(out/'mutant-bindings.c').write_text(q)
retail=Path('disc/SLUS_006.64').read_bytes()[0x80025258-0x8000f800:0x8002541c-0x8000f800]
assert sha256(retail).hexdigest()=='d78069fc2f793a0e19306def3c307454192f0fd3eab8c53a7f95d24acb992737'
files=['src/slus_006.64/system/temp1e.c','pc_port/src/game_overrides.c','pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','tools/scripts/gen_battle_bridge_map.py','config/symbol_addrs.slus_006.64.txt','linker/undefined_funcs_auto.battle.txt','linker/undefined_syms_auto.battle.txt','config/symbol_addrs.battle.txt','pc_port/tests/battle_child_billboard_retail_test.c','pc_port/tests/run_battle_child_billboard_retail_test.sh']
proof={'retail_80025258_8002541c_sha256':sha256(retail).hexdigest(),'source_sha256':{x:sha256(Path(x).read_bytes()).hexdigest() for x in files},'generated_map_sha256':sha256((out/'battle_bridge_map.inc').read_bytes()).hexdigest(),'verbatim_fixture_sha256':sha256(p.encode()).hexdigest(),'controlled_leaves':['SetRotMatrix','SetTransMatrix','RotTransPers','TransMatrix','func_80022038','func_8001E3D8','func_8001E298']}
(out/'provenance.json').write_text(json.dumps(proof,indent=2)+'\n')
PY
common=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT" -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -O0 -Wall -Wextra -Werror)
build_positive() {
 local mode=$1 opt=$2 sanitize=$3
 clang "${common[@]}" "$opt" $sanitize -c pc_port/tests/battle_child_billboard_retail_test.c -o "$OUT/test-$mode.o"
 clang -std=gnu17 -fno-pie "$opt" $sanitize -c pc_port/src/battle_mips_adapter.c -o "$OUT/cpu-$mode.o"
 clang -std=gnu17 -fno-pie -DXENO_PC_PORT -Ipc_port/src "$opt" $sanitize -Wno-incompatible-pointer-types -c "$OUT/production.c" -o "$OUT/prod-$mode.o"
 clang -no-pie $sanitize -Wl,--export-dynamic "$OUT/test-$mode.o" "$OUT/cpu-$mode.o" "$OUT/prod-$mode.o" -ldl -o "$OUT/test-$mode"
 "$OUT/test-$mode" | tee "$OUT/$mode.log"
}
build_positive O0 -O0 ''
build_positive O2 -O2 ''
echo "BATTLE CHILD BILLBOARD UBSan regime: clang (-fsanitize=undefined; gcc cannot link libubsan on this host)"
build_positive UBSan -O1 '-fsanitize=undefined -fno-sanitize=function -fno-sanitize-recover=all'
for source in "$OUT"/mutant-*.c; do
 name=${source##*/};name=${name%.c}
 clang -std=gnu17 -fno-pie -DXENO_PC_PORT -Ipc_port/src -O0 -Wno-incompatible-pointer-types -c "$source" -o "$OUT/$name.o"
 clang -no-pie -Wl,--export-dynamic "$OUT/test-O0.o" "$OUT/cpu-O0.o" "$OUT/$name.o" -ldl -o "$OUT/test-$name"
 if "$OUT/test-$name" >"$OUT/$name.log" 2>&1; then echo "$name unexpectedly passed" >&2;exit 1;fi
 echo "BATTLE CHILD BILLBOARD mutant rejected: $name"
done
