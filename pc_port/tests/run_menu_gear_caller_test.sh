#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_gear_caller
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import hashlib
b=Path('disc/menu.bin').read_bytes()
import re
for addr,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',next(Path('asm/menu').glob('*matchings/main/misc*/func_801DFE2C.s')).read_text()):
 off=int(addr,16)-0x801c5000
 assert b[off:off+4]==bytes.fromhex(h)
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'

PYGEN
python3 - <<'PYGEN'
from pathlib import Path
s=Path('src/menu/main/misc.c').read_text();a=s.index('void func_801DFE2C(u8 slotIdx) {');z=s.index('\n}\n',a)+3
Path('pc_port/build_native/menu_gear_caller/owner.c').write_text('#include "common.h"\n#include "system/menu.h"\n#include "main/game.h"\nextern void func_801E3ECC(void*,u8);\nextern void func_801E3C2C(void*,u8);\n'+s[a:z])
PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan;do
 flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all);fi
 gcc "${BASE[@]}" "${flags[@]}" -c "$OUT/owner.c" -o "$OUT/$mode.owner.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/tests/menu_gear_caller_test.c -o "$OUT/$mode.test.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.owner.o" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
 "$OUT/$mode" | tee "$OUT/$mode.log"
done
python3 - <<'PYGEN'
from pathlib import Path
p=Path('pc_port/build_native/menu_gear_caller');s=(p/'owner.c').read_text()
mutants={'stride':('* 0xA4','* 0x28'),'reload':('resource = g_Menu->unk330;','resource = (MenuUnk6*)0;'),'copy':('source + 0x90','source + 0x84'),'byte_width':('source[0x92]','*(u16*)(source + 0x92)')}
# Reload mutant keeps the resource from before the second callback.
mutants['reload']=('func_801E3C2C(g_Menu->unk330, gearId);','resource = g_Menu->unk330; func_801E3C2C(g_Menu->unk330, gearId);')
for n,(old,new) in mutants.items():
 assert old in s
 b=s.replace(old,new)
 if n=='reload':b=b.replace('    resource = g_Menu->unk330;\n    source', '    source')
 (p/(n+'.c')).write_text(b)
PYGEN
for mutant in stride reload copy byte_width;do
 gcc "${BASE[@]}" -O2 -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant"
 if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1;then echo "FAIL survived $mutant";exit 1;fi
 grep -q '^FAIL gear caller' "$OUT/$mutant.log"
done
echo 'GEAR CALLER negative controls PASS count=4'
