#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_equip_resources
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import hashlib
b=Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
assert hashlib.sha256(b[0x22bc:0x2b0c]).hexdigest()=='2f315236e4074f75cf1121fc8e865958635fabf229c3229552d49636857971c1'
PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan;do
 flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all);fi
 gcc "${BASE[@]}" "${flags[@]}" -c src/menu/main/misc.c -o "$OUT/$mode.owner.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/tests/menu_equip_resources_test.c -o "$OUT/$mode.test.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.owner.o" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
 "$OUT/$mode" | tee "$OUT/$mode.log"
done

python3 - <<'PYGEN'
from pathlib import Path
p=Path('pc_port/build_native/menu_equip_resources');s=Path('src/menu/main/misc.c').read_text();a=s.index('void func_801C72BC(s32 mode)');z=s.index('\n#endif',a);b=s[a:z]
mutants={
 'weapon_entry':('archive[2], 0','archive[3], 0'),
 'extra_bank':('archive[0x2B], 0','archive[0x2A], 0'),
 'desc_bank7':('archive[0x35], 0','archive[0x34], 0'),
 'free17_order':(
  'memcpy(&token, listBuf + 0xA00, 4);\n        HeapFree((void*)(uintptr_t)token);\n        memcpy(&token, listBuf + 0xA04, 4);\n        HeapFree((void*)(uintptr_t)token);',
  'memcpy(&token, listBuf + 0xA04, 4);\n        HeapFree((void*)(uintptr_t)token);\n        memcpy(&token, listBuf + 0xA00, 4);\n        HeapFree((void*)(uintptr_t)token);'),
}
for n,(old,new) in mutants.items():
 assert b.count(old)==1, (n, old, b.count(old))
 (p/(n+'.c')).write_text(s[:a]+b.replace(old,new)+s[z:])
PYGEN
for mutant in weapon_entry extra_bank desc_bank7 free17_order;do
 gcc "${BASE[@]}" -O2 -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant"
 if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1;then echo "FAIL survived $mutant";exit 1;fi
 grep -q '^FAIL equip resource' "$OUT/$mutant.log"
done
echo 'EQUIP RESOURCES negative controls PASS count=4'
