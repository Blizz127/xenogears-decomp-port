#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mktemp -d pc_port/build_native/atlas_work_buffer.XXXXXXXX)
python3 - "$out" <<'PY'
from pathlib import Path
from hashlib import sha256
import sys
out=Path(sys.argv[1]);s=Path('src/slus_006.64/system/temp1f.c').read_text();start=s.index('void func_80026BA4(');end=s.index('\n}\n',start)+3;s=s[start:end]
retail=Path('disc/SLUS_006.64').read_bytes();assert sha256(retail).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
old='u8* pPrimBuffer)' in s
header='#include <stdint.h>\ntypedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int16_t s16;typedef int32_t s32;\nextern void *g_GfxCurWorkBuffer,*g_GfxCurWorkBufferEnd;extern u8 *g_GfxCurOT;int GetClut(int,int);int GetTPage(int,int,int,int);void AddPrim(void*,void*);\n'
call='func_80026BA4(t,i,x,y,(s16)(uintptr_t)o,trap);' if old else 'func_80026BA4(t,i,x,y,o);'
(out/'native.c').write_text(header+s+'\nvoid atlas_test_call(u8*t,s32 i,s32 x,s32 y,void*o,void*trap){(void)trap;'+call+'}\n')
(out/'pins.txt').write_text('SLUS '+sha256(retail).hexdigest()+'\n80026BA4..80026DCC '+sha256(retail[0x26ba4-0xf800:0x26dcc-0xf800]).hexdigest()+'\nsource '+sha256(s.encode()).hexdigest()+'\n')
PY
CC="${CC:-cc}"
for mode in O0 O2 ubsan;do
 flags=(-O0);if [ "$mode" = O2 ];then flags=(-O2);fi
 if [ "$mode" = ubsan ];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 "$CC" -std=c11 -g -no-pie "${flags[@]}" -Ipc_port/src "$out/native.c" pc_port/tests/atlas_work_buffer_retail_test.c pc_port/src/battle_mips_adapter.c -o "$out/$mode"
 "$out/$mode"
done
python3 - "$out" <<'PYCODE'
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=(p/'native.c').read_text()
for name,old,new in [('exact_fit','>=\n','>\n'),('cursor','g_GfxCurWorkBuffer = poly + 40;','g_GfxCurWorkBuffer = poly;'),('tpage','GetTPage(mode, 0, texX, texY)','GetTPage(mode, 0, *(s16*)(item + 18), *(s16*)(item + 20))'),('uv','poly[12] = poly[28] = u;','poly[12] = poly[28] = texX;')]:
 assert s.count(old)==1,(name,s.count(old));(p/(name+'.c')).write_text(s.replace(old,new))
PYCODE
for mutant in exact_fit cursor tpage uv;do
 "$CC" -std=c11 -O2 -no-pie -Ipc_port/src "$out/$mutant.c" pc_port/tests/atlas_work_buffer_retail_test.c pc_port/src/battle_mips_adapter.c -o "$out/$mutant"
 if "$out/$mutant" > "$out/$mutant.log" 2>&1;then echo "MUTANT SURVIVED: $mutant";exit 1;fi
 echo "MUTANT REJECTED: $mutant"
done
echo "Artifacts: $out"
