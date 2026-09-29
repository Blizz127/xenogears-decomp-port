#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_e7_retail_test.XXXXXXXX)
export TEST_OUT
printf 'SPRITE E7 artifacts: %s\n' "$TEST_OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import os,json,struct
out=Path(os.environ['TEST_OUT']);b=Path('disc/SLUS_006.64').read_bytes()
assert sha256(b).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
pins={}
for name,a,z,h in [
 ('dispatcher',0x8001fbe4,0x80021ad8,'7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c'),
 ('scale',0x80022000,0x80022038,'daa73a955905fe34d31204c01207f55d4da7dadd4419a5437ba90afcd23d70db')]:
 assert sha256(b[a-0x8000f800:z-0x8000f800]).hexdigest()==h
 pins[name]={'start':hex(a),'end':hex(z),'sha256':h}
assert struct.unpack_from('<I',b,0x800183d8+(0xe7-0x8a)*4-0x8000f800)[0]==0x80021340
pins['handler_sha256']=sha256(b[0x80021340-0x8000f800:0x80021374-0x8000f800]).hexdigest()
for rel in [os.environ.get('SPRITE_E7_SOURCE','src/slus_006.64/system/animation_scripts.c'),'pc_port/tests/sprite_dispatch_e7_retail_test.c','pc_port/tests/run_sprite_dispatch_e7_retail_test.sh','pc_port/src/battle_mips_adapter.c']:
 data=Path(rel).read_bytes();pins[rel]=sha256(data).hexdigest();(out/Path(rel).name).write_bytes(data)
(out/'provenance.json').write_text(json.dumps(pins,indent=2)+'\n')
PY
build_case() {
 local name=$1 opt=$2 source=${3:-${SPRITE_E7_SOURCE:-src/slus_006.64/system/animation_scripts.c}}
 local -a flags common
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
 gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -c "$source" -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_e7_retail_test.c pc_port/src/battle_mips_adapter.c "$TEST_OUT/$name.body.o" -o "$TEST_OUT/$name.test" >> "$TEST_OUT/$name.build.log" 2>&1
}
red=0
for opt in O0 O2 UBSan; do
 build_case "$opt" "$opt"
 status=0
 "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
 if [ "$status" -eq 0 ]; then cat "$TEST_OUT/$opt.log"
 elif rg -q 'sprite_animation_unimplemented.*"opcode":231' "$TEST_OUT/$opt.log"; then
  red=1; printf 'SPRITE E7 %s semantic RED: unsupported retail opcode\n' "$opt"
 else cat "$TEST_OUT/$opt.log" >&2; exit 2
 fi
done
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json,os
pins=json.loads((Path(os.environ['TEST_OUT'])/'provenance.json').read_text())
for rel,h in pins.items():
 if '/' in rel: assert sha256(Path(rel).read_bytes()).hexdigest()==h,rel
PY
if [ "$red" -eq 1 ]; then exit 1; fi
python3 - <<'PYCONTROLS'
from pathlib import Path
import os,json
out=Path(os.environ['TEST_OUT']);source=Path(os.environ.get('SPRITE_E7_SOURCE','src/slus_006.64/system/animation_scripts.c')).read_text()
a=source.index('    case 0xE7: {');b=source.index('    case 0xF2: {',a);body=source[a:b]
mutants={
 'missing_double':body.replace('(packed << 1)','packed'),
 'wrong_scale_field':body.replace('p + 0x2C','p + 0x2E'),
 'missing_existing_scale':body.replace('(s16)(scale + delta)','(s16)delta'),
 'swapped_operand_bytes':body.replace('source[1]','source[0]').replace('source[0] |','source[1] |'),
 'neighbor_corruption':body.replace('        return;', '        p[0x2E] = 0;\n        return;'),
 'missing_helper':body.replace('SpriteSetScale((SpriteData*)p, (s16)(scale + delta));','(void)p;'),
}
for name,changed in mutants.items():
 assert changed!=body,name
 (out/(name+'.c')).write_text(source[:a]+changed+source[b:])
(out/'controls.txt').write_text('\n'.join(mutants)+'\n')
PYCONTROLS
while IFS= read -r name; do
 build_case "$name" O2 "$TEST_OUT/$name.c"
 status=0
 "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
 if [ "$status" -ne 1 ] || ! rg -q '^SPRITE E7 FAIL case=' "$TEST_OUT/$name.log"; then
  cat "$TEST_OUT/$name.log" >&2; printf 'SPRITE E7 failed control: %s rc=%s\n' "$name" "$status" >&2; exit 2
 fi
 printf 'SPRITE E7 control rejected %s\n' "$name"
done < "$TEST_OUT/controls.txt"
python3 - <<'PYEND'
from pathlib import Path
from hashlib import sha256
import json,os
out=Path(os.environ['TEST_OUT']);pins=json.loads((out/'provenance.json').read_text())
for rel,h in pins.items():
 if '/' in rel: assert sha256(Path(rel).read_bytes()).hexdigest()==h,rel
PYEND
printf 'SPRITE E7 GREEN: O0/O2/UBSan positive census and six semantic controls\n'
