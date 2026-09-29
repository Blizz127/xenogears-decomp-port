#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_c5_retail_test.XXXXXXXX)
export TEST_OUT
printf 'SPRITE C5 artifacts: %s\n' "$TEST_OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import os,json,struct
out=Path(os.environ['TEST_OUT']);b=Path('disc/SLUS_006.64').read_bytes()
assert sha256(b).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
pins={}
for name,a,z,h in [
 ('dispatcher',0x8001fbe4,0x80021ad8,'7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c'),
 ('handler',0x8001fcac,0x8001fcf0,'6836927bde0da7551edb02f4ebdbe04609e206a277a44efe630bb8d7992419b6')]:
 assert sha256(b[a-0x8000f800:z-0x8000f800]).hexdigest()==h
 pins[name]={'start':hex(a),'end':hex(z),'sha256':h}
assert struct.unpack_from('<I',b,0x800183d8+(0xc5-0x8a)*4-0x8000f800)[0]==0x8001fcac
pins['handler_sha256']=sha256(b[0x8001fcac-0x8000f800:0x8001fcf0-0x8000f800]).hexdigest()
for rel in [os.environ.get('SPRITE_C5_SOURCE','src/slus_006.64/system/animation_scripts.c'),'pc_port/tests/sprite_dispatch_c5_retail_test.c','pc_port/tests/run_sprite_dispatch_c5_retail_test.sh','pc_port/src/battle_mips_adapter.c']:
 data=Path(rel).read_bytes();pins[rel]=sha256(data).hexdigest();(out/Path(rel).name).write_bytes(data)
(out/'provenance.json').write_text(json.dumps(pins,indent=2)+'\n')
PY
build_case() {
 local name=$1 opt=$2 source=${3:-${SPRITE_C5_SOURCE:-src/slus_006.64/system/animation_scripts.c}}
 local -a flags common
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
 gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -c "$source" -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_c5_retail_test.c pc_port/src/battle_mips_adapter.c "$TEST_OUT/$name.body.o" -o "$TEST_OUT/$name.test" >> "$TEST_OUT/$name.build.log" 2>&1
}
red=0
for opt in O0 O2 UBSan; do
 build_case "$opt" "$opt"
 status=0
 "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
 if [ "$status" -eq 0 ]; then cat "$TEST_OUT/$opt.log"
 elif rg -q 'sprite_animation_unimplemented.*"opcode":197' "$TEST_OUT/$opt.log"; then
  red=1; printf 'SPRITE C5 %s semantic RED: unsupported retail opcode\n' "$opt"
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
import os
out=Path(os.environ['TEST_OUT']);source=Path('src/slus_006.64/system/animation_scripts.c').read_text()
a=source.index('    if ((u8)opcodeIndex == 0xC5) {');b=source.index('    if ((u8)opcodeIndex == 0xB5)',a);body=source[a:b]
mutants={
 'signed_operand':body.replace('((u8*)operands)[0]','(s8)((u8*)operands)[0]'),
 'wrong_divisor':body.replace('(u32)D_80059198 + 1u','(u32)D_80059198 + 2u'),
 'extra_call':body.replace('u32 remaining = (u32)(divisor == 0 ? -1 : numerator / divisor);','u32 remaining = (u32)(divisor == 0 ? -1 : numerator / divisor) + 1u;'),
 'missing_call':body.replace('func_80022CDC(pSpriteData);','(void)pSpriteData;'),
}
for name,changed in mutants.items():
 assert changed!=body,name
 (out/(name+'.c')).write_text(source[:a]+changed+source[b:])
(out/'controls.txt').write_text('\n'.join(mutants)+'\n')
PYCONTROLS
while IFS= read -r name; do
 build_case "$name" O2 "$TEST_OUT/$name.c"
 status=0
 timeout 10 "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
 if [ "$status" -ne 1 ] || ! rg -q '^SPRITE C5 FAIL case=' "$TEST_OUT/$name.log"; then
  cat "$TEST_OUT/$name.log" >&2; printf 'SPRITE C5 failed control: %s rc=%s\n' "$name" "$status" >&2; exit 2
 fi
 printf 'SPRITE C5 control rejected %s\n' "$name"
done < "$TEST_OUT/controls.txt"
