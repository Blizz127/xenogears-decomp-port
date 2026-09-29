#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_gear_stats
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import hashlib
b=Path('disc/menu.bin').read_bytes()
import re
for addr,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',next(Path('asm/menu').glob('*matchings/main/misc*/func_801E3C2C.s')).read_text()):
 off=int(addr,16)-0x801c5000
 assert b[off:off+4]==bytes.fromhex(h)
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'

PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan;do
 flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all);fi
 gcc "${BASE[@]}" "${flags[@]}" -c src/menu/main/misc.c -o "$OUT/$mode.owner.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/tests/menu_gear_stats_test.c -o "$OUT/$mode.test.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.owner.o" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
 "$OUT/$mode" | tee "$OUT/$mode.log"
done
python3 - <<'PYGEN'
from pathlib import Path
p=Path('pc_port/build_native/menu_gear_stats');s=Path('src/menu/main/misc.c').read_text();a=s.index('void func_801E3C2C(void* resourceArg');z=s.index('\n#endif',a);b=s[a:z]
mutants={'hp_scale':('* 50;','* 25;'),'division':('* 6 / 10','* 6 / 5'),'map_flag':('D_801E9808[9] = 10','D_801E9808[8] = 10'),'subtraction':('gear[0x98] - gear[0x4A]','gear[0x98] + gear[0x4A]')}
for n,(old,new) in mutants.items():
 assert old in b
 (p/(n+'.c')).write_text(s[:a]+b.replace(old,new)+s[z:])
PYGEN
for mutant in hp_scale division map_flag subtraction;do
 gcc "${BASE[@]}" -O2 -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant"
 if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1;then echo "FAIL survived $mutant";exit 1;fi
 grep -q '^FAIL gear stats' "$OUT/$mutant.log"
done
echo 'GEAR STATS negative controls PASS count=4'
