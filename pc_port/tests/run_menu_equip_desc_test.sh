#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_equip_desc
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import hashlib, re
b=Path('disc/menu.bin').read_bytes()
words=list(re.finditer(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',next(Path('asm/menu').glob('*matchings/main/misc*/func_801DFF5C.s')).read_text()))
for m in words:
 off=int(m.group(1),16)-0x801c5000
 assert b[off:off+4]==bytes.fromhex(m.group(2)), (m.group(1),m.group(2))
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
print('DFF5C instruction words OK, bytes', len(words)*4)
PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -fno-strict-aliasing -DXENO_PC_PORT -DXENO_EQUIP_DESC_TEST -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 UBSan; do
  flags=(-O0); if [[ "$mode" == UBSan ]]; then flags=(-O0 -fsanitize=undefined -fno-sanitize-recover=all); fi
  gcc "${BASE[@]}" "${flags[@]}" -c src/menu/main/misc.c -o "$OUT/$mode.owner.o"
  gcc "${BASE[@]}" "${flags[@]}" -c pc_port/tests/menu_equip_desc_test.c -o "$OUT/$mode.test.o"
  gcc "${BASE[@]}" -O0 -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
  clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.owner.o" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
  "$OUT/$mode" | tee "$OUT/$mode.log"
done
python3 - <<'PYGEN'
from pathlib import Path
p=Path('pc_port/build_native/menu_equip_desc')
s=Path('src/menu/main/misc.c').read_text()
a=s.index('void func_801DFF5C(s32 categoryArg')
z=s.index('\n}\n', a) + 3  # end of the function body (column-0 brace)
b=s[a:z]
mutants={
 'kind_bias':('kind = (u8)(kind + (gearMode << 1))','kind = (u8)(kind + gearMode)'),
 'entry_stride':('s32 entryBase = itemId * 3','s32 entryBase = itemId * 2'),
 'visible_flag':('listBuf[0xA18] = 1','listBuf[0xA18] = 0'),
 'equip_weapon':('itemId = state[0x2D6 + base]','itemId = state[0x2D7 + base]'),
}
for n,(old,new) in mutants.items():
 assert b.count(old)==1, (n,old,b.count(old))
 (p/(n+'.c')).write_text(s[:a]+b.replace(old,new)+s[z:])
PYGEN
for mutant in kind_bias entry_stride visible_flag equip_weapon; do
  gcc "${BASE[@]}" -O0 -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
  clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O0.test.o" "$OUT/O0.mips.o" -o "$OUT/$mutant"
  if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1; then echo "FAIL survived $mutant"; exit 1; fi
  grep -q '^FAIL' "$OUT/$mutant.log"
done
echo 'EQUIP DESC negative controls PASS count=4'
