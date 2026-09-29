#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_equip_stats
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import hashlib
b=Path('disc/menu.bin').read_bytes()
import re
for addr,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',next(Path('asm/menu').glob('*matchings/main/misc*/func_801E3A80.s')).read_text()):
 off=int(addr,16)-0x801c5000
 assert b[off:off+4]==bytes.fromhex(h)
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'

PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan;do
 flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all);fi
 gcc "${BASE[@]}" "${flags[@]}" -c src/menu/main/misc.c -o "$OUT/$mode.owner.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/tests/menu_equip_stats_test.c -o "$OUT/$mode.test.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.owner.o" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
 "$OUT/$mode" | tee "$OUT/$mode.log"
done
python3 - <<'PYGEN'
from pathlib import Path
p=Path('pc_port/build_native/menu_equip_stats');s=Path('src/menu/main/misc.c').read_text();a=s.index('void func_801E3A80(void* pCtx');z=s.index('\nINCLUDE_ASM(',a);b=s[a:z]
# one definition may carry the retail body behind an inner #else: mutate only
# the port arm (depth-aware, the port arm has its own nested #ifdef)
if '\n#ifdef XENO_PC_PORT\n' in b[:120]:
 i=s.index('\n#ifdef XENO_PC_PORT\n',a)+1;d=0;pos=i
 for line in s[i:].split('\n')[1:]:
  pos=s.index('\n',pos)+1;t=line.strip()
  if t.startswith('#if'): d+=1
  elif t.startswith('#endif'): d-=1
  elif t.startswith('#else') and d==0: z=pos-1; break
 b=s[a:z]
mutants={'layout':('(u8*)&((MenuUnk6*)pCtx)->unkB8','(u8*)pCtx + 0xB8'),'division':('* 6 / 10','* 6 / 5'),'cap':('= 0x10;','= 0x14;'),'bonus':('pEntry[0x5E] + pEntry[0x2E]','pEntry[0x5E] + pEntry[0x2F]')}
for n,(old,new) in mutants.items():
 assert b.count(old)==1
 (p/(n+'.c')).write_text(s[:a]+b.replace(old,new)+s[z:])
PYGEN
for mutant in layout division cap bonus;do
 gcc "${BASE[@]}" -O2 -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant"
 if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1;then echo "FAIL survived $mutant";exit 1;fi
 grep -q '^FAIL equip stats' "$OUT/$mutant.log"
done
echo 'EQUIP STATS negative controls PASS count=4'
