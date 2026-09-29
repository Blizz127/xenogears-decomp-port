#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_equip_preview
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import hashlib
b=Path('disc/menu.bin').read_bytes()
import re
for addr,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',next(Path('asm/menu').glob('*matchings/main/misc*/func_801DFB68.s')).read_text()):
 off=int(addr,16)-0x801c5000
 assert b[off:off+4]==bytes.fromhex(h)
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'

PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan;do
 flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all);fi
 gcc "${BASE[@]}" "${flags[@]}" -c src/menu/main/misc.c -o "$OUT/$mode.owner.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/tests/menu_equip_preview_test.c -o "$OUT/$mode.test.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.owner.o" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
 "$OUT/$mode" | tee "$OUT/$mode.log"
done
python3 - <<'PYGEN'
from pathlib import Path
p=Path('pc_port/build_native/menu_equip_preview');s=Path('src/menu/main/misc.c').read_text();a=s.index('void func_801DFB68(s32 slotArg');z=s.index('\n#endif',a);b=s[a:z]
mutants={'category_gate':('category >= 4','category >= 3'),'character_bank':('0x2DF + category','0x2DE + category'),'gear_bank':('0x97C + category','0x97D + category'),'scroll':('(u32)row + (u32)scroll','((u32)row + (u32)scroll) ^ 1')}
for n,(old,new) in mutants.items():
 assert b.count(old)==1
 (p/(n+'.c')).write_text(s[:a]+b.replace(old,new)+s[z:])
PYGEN
for mutant in category_gate character_bank gear_bank scroll;do
 gcc "${BASE[@]}" -O2 -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant"
 if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1;then echo "FAIL survived $mutant";exit 1;fi
 grep -q '^FAIL equip preview' "$OUT/$mutant.log"
done
echo 'EQUIP PREVIEW negative controls PASS count=4'
