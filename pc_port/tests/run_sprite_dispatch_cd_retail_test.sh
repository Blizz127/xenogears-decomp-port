#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_dispatch_cd_retail_test
mkdir -p "$OUT"
python3 - <<'CHECK'
from hashlib import sha256
from pathlib import Path
assert sha256(Path('disc/SLUS_006.64').read_bytes()).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
CHECK
for opt in O0 O2 UBSan; do
 flags=(-"$opt"); if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc -std=gnu17 -fpermissive "${flags[@]}" -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c src/slus_006.64/system/animation_scripts.c -o "$OUT/$opt.body.o"
 # Borrow dependency guards: any unexpected helper call aborts the fixture.
 gcc -std=gnu17 "${flags[@]}" -Dmain=noop_main -fno-pie -Ipc_port/src -c pc_port/tests/sprite_dispatch_noop_retail_test.c -o "$OUT/$opt.guards.o"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${flags[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_cd_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" "$OUT/$opt.guards.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
echo 'SPRITE CD O0/O2/UBSAN PASS'

python3 - <<'PY'
from pathlib import Path
s=Path('src/slus_006.64/system/animation_scripts.c').read_text();a=s.index('    case 0xCD: {');b=s.index('    case 0xCC:',a);body=s[a:b]
for name,old,new in [('angle-scale','<< 3','<< 2'),('index-mask','& 7u','& 3u'),('assign-bit','operand & 0x1000u','operand & 0x2000u'),('direction-offset','index * 8u + 2u','index * 8u + 0u'),('model-dirty','if (index == 0)','if (index == 7)')]:
 assert body.count(old)==1,name
 Path('pc_port/build_native/sprite_dispatch_cd_retail_test',name+'.c').write_text(s[:a]+body.replace(old,new)+s[b:])
PY
for mutant in angle-scale index-mask assign-bit direction-offset model-dirty;do
 gcc -std=gnu17 -fpermissive -O2 -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie -O2 -Wl,--gc-sections pc_port/tests/sprite_dispatch_cd_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.o" "$OUT/O2.guards.o" -o "$OUT/$mutant.test"
 rc=0
 "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! rg -q 'SPRITE CD FAIL case=' "$OUT/$mutant.log";then cat "$OUT/$mutant.log";exit 1;fi
 echo "CD MUTANT REJECTED $mutant"
done
