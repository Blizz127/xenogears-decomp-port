#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_equip_list
mkdir -p "$OUT"
LINK_CC=${LINK_CC:-clang}
python3 - <<'PYGEN'
from pathlib import Path
import hashlib, re
b=Path('disc/menu.bin').read_bytes()
for addr,h in re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',next(Path('asm/menu').glob('*matchings/main/misc*/func_801DE5CC.s')).read_text()):
 off=int(addr,16)-0x801c5000
 assert b[off:off+4]==bytes.fromhex(h), (addr,h)
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
print('DE5CC instruction words OK, bytes', sum(1 for _ in re.finditer(r'/\* \w+ [0-9A-F]{8} [0-9A-F]{8} \*/',next(Path('asm/menu').glob('*matchings/main/misc*/func_801DE5CC.s')).read_text()))*4)
PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -fno-strict-aliasing -DXENO_PC_PORT -DXENO_EQUIP_LIST_TEST -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan; do
  flags=(-"$mode"); if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
  gcc "${BASE[@]}" "${flags[@]}" -c src/menu/main/misc.c -o "$OUT/$mode.owner.o"
  gcc "${BASE[@]}" "${flags[@]}" -c pc_port/tests/menu_equip_list_test.c -o "$OUT/$mode.test.o"
  gcc "${BASE[@]}" -O0 -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
  "$LINK_CC" -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.owner.o" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
  "$OUT/$mode" | tee "$OUT/$mode.log"
done
# Mutants must also be rejected with the optimized owner and harness.
python3 - <<'PYGEN'
from pathlib import Path
p=Path('pc_port/build_native/menu_equip_list')
s=Path('src/menu/main/misc.c').read_text()
a=s.index('s32 func_801DE5CC(s32 selectedArg')
z=s.index('\n#endif\n\n#ifndef XENO_PC_PORT\nINCLUDE_ASM("../asm/menu/nonmatchings/main/misc", func_801DF0D4)', a)
b=s[a:z]
mutants={
 'weapon_id_gate':('charWeaponIds[i] < 0x32','charWeaponIds[i] < 0x31'),
 'accessory_start':('filled = isAccessory','filled = 0'),
 'gear_stride':('gearWeaponIds[i] * 20','gearWeaponIds[i] * 16'),
 'return_bias':('filled -= 8','filled -= 7'),
}
for n,(old,new) in mutants.items():
 assert b.count(old)==1, (n,old,b.count(old))
 (p/(n+'.c')).write_text(s[:a]+b.replace(old,new)+s[z:])
PYGEN
for mutant in weapon_id_gate accessory_start gear_stride return_bias; do
  gcc "${BASE[@]}" -O2 -DXENO_EQUIP_LIST_TEST -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
  "$LINK_CC" -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant"
  if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1; then echo "FAIL survived $mutant"; exit 1; fi
  grep -q '^FAIL' "$OUT/$mutant.log"
done
# Regression control: the former native-size guest mapping must be rejected.
python3 - <<'PYMAP'
from pathlib import Path
s = Path('pc_port/tests/menu_equip_list_test.c').read_text()
old = 'sizeof retailMenu)\n        p = retailMenu + (a - 0x80100000);'
new = 'sizeof menu)\n        p = (u8*)&menu + (a - 0x80100000);'
assert s.count(old) == 1
Path('pc_port/build_native/menu_equip_list/guest_overlap.c').write_text(s.replace(old, new))
PYMAP
gcc "${BASE[@]}" -O2 -c "$OUT/guest_overlap.c" -o "$OUT/guest_overlap.o"
"$LINK_CC" -no-pie -Wl,--gc-sections "$OUT/O2.owner.o" "$OUT/guest_overlap.o" "$OUT/O2.mips.o" -o "$OUT/guest_overlap"
if "$OUT/guest_overlap" >"$OUT/guest_overlap.log" 2>&1; then
  echo 'FAIL survived guest menu overlap'; exit 1
fi
grep -q '^FAIL guest character/Gear mapping' "$OUT/guest_overlap.log"
echo 'EQUIP LIST negative controls PASS count=5'
