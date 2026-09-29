#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/battle_encounter_alias_test.XXXXXXXX)
export TEST_OUT
echo "BATTLE ENCOUNTER ALIAS artifacts: $TEST_OUT"

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os, struct
out = Path(os.environ['TEST_OUT']); pins = {}
for name, expected in [
 ('disc/battle.bin','1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'),
 ('disc/SLUS_006.64','dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119')]:
 actual=sha256(Path(name).read_bytes()).hexdigest(); assert actual==expected,(name,actual); pins[name]=actual
battle=Path('disc/battle.bin').read_bytes(); span=battle[0x1640:0x1668]
assert span.hex()=='0780043cdcf98424200006340680053c0895a5900680023cdc5842244029050067fe000c2128a200'
pins['retail_copy_80071130_80071154']=sha256(span).hexdigest()
with open('disc/disc1.bin','rb') as disc:
 map2=bytearray()
 for off in range(0,123808,2048):
  disc.seek((121096+off//2048)*2352+24); map2.extend(disc.read(min(2048,123808-off)))
assert sha256(map2).hexdigest()=='d81670fa78aeed510852153349cd8d4e4b5a6bc6469110e41fb6274aa4e4261f'
source_offset=struct.unpack_from('<I',map2,0x148)[0]; assert source_offset==121448
source=memoryview(map2)[source_offset:]; size=struct.unpack_from('<I',source)[0]; assert size==530
cursor=4; decoded=bytearray()
while len(decoded)<size:
 flags=source[cursor]; cursor+=1
 for _ in range(8):
  if len(decoded)==size: break
  if flags&1:
   b0,b1=source[cursor],source[cursor+1]; cursor+=2; distance=b0|((b1&15)<<8)
   for _ in range((b1>>4)+3): decoded.append(decoded[-distance])
  else: decoded.append(source[cursor]); cursor+=1
  flags>>=1
assert sha256(decoded).hexdigest()=='727bad94d380db9065d190fb7c28cc422549030dcaab9edee894b186fbbf85e9'
record=bytes(decoded[0x20:0x40]); assert record[2]==0x12
assert sha256(record).hexdigest()=='a950f44cf9683f8c84d0aee2371207721f38857fbcd84c3b20bd2932732b6eec'
(out/'map2-selector1-record.bin').write_bytes(record)
pins['map2_selector1']={'sector':121096,'map_size':123808,'compressed_offset':source_offset,
 'decoded_sha256':sha256(decoded).hexdigest(),'record_offset':32,'record_sha256':sha256(record).hexdigest()}
for name in ['pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c',
 'src/slus_006.64/system/temp3.c','config/symbol_addrs.slus_006.64.txt',
 'linker/undefined_funcs_auto.battle.txt','linker/undefined_syms_auto.battle.txt',
 'tools/scripts/gen_battle_bridge_map.py','pc_port/tests/battle_encounter_alias_test.c',
 'pc_port/tests/run_battle_encounter_alias_test.sh']:
 pins[name]=sha256(Path(name).read_bytes()).hexdigest()
(out/'provenance.json').write_text(json.dumps(pins,indent=2)+'\n')
temp3=Path('src/slus_006.64/system/temp3.c').read_text(); start=temp3.index('extern u8 D_8006F9DE;')
end=temp3.index('\n}',temp3.index('s32 func_8001BB0C(void)',start))+2
preamble='#include <stdint.h>\n#include "psx_memory.h"\ntypedef uint8_t u8;\ntypedef int32_t s32;\n'
production=preamble+temp3[start:end]+'\n'; needle='*(u8*)PSX_ADDR(0x8006F9DEu)'; assert production.count(needle)==1
(out/'consumer-production.c').write_text(production)
mutants={'separate-stub':production.replace(needle,'D_8006F9DE'),
 'wrong-offset-plus1':production.replace(needle,'*(u8*)PSX_ADDR(0x8006F9DDu)'),
 'wrong-offset-plus3':production.replace(needle,'*(u8*)PSX_ADDR(0x8006F9DFu)')}
manifest={'production_sha256':sha256(production.encode()).hexdigest(),'mutants':{}}
for name,text in mutants.items():
 assert text!=production; (out/f'consumer-{name}.c').write_text(text); manifest['mutants'][name]=sha256(text.encode()).hexdigest()
(out/'negative-controls.json').write_text(json.dumps(manifest,indent=2)+'\n')
PY

bridge_elf=(); if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
 --symbols config/symbol_addrs.slus_006.64.txt --symbols linker/undefined_funcs_auto.battle.txt \
 --symbols linker/undefined_syms_auto.battle.txt --out "$TEST_OUT/battle_bridge_map.inc"
rg -q '0x800658dcu, 512u, 0, "D_800658DC"' "$TEST_OUT/battle_bridge_map.inc"
rg -q '0x8006f9dcu, 0u, 0, "D_8006F9DC"' "$TEST_OUT/battle_bridge_map.inc"
rg -q '0x8006f9ddu, 0u, 0, "D_8006F9DD"' "$TEST_OUT/battle_bridge_map.inc"
printf '%s\n' '#include <stdint.h>' 'uint8_t D_8006F9DE = 0xe7;' > "$TEST_OUT/generated-stub.c"

common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM
 -D_LANGUAGE_C -include assert.h -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
 -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$TEST_OUT" -Wall -Wextra -Werror)
failed=0
for opt in O0 O2 UBSan; do
 flags=(-"$opt"); if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -c pc_port/tests/battle_encounter_alias_test.c -o "$TEST_OUT/$opt.test.o"
 gcc -std=gnu17 -fno-pie -ffunction-sections -fdata-sections "${flags[@]}" -Wall -Wextra -Werror \
  -c pc_port/src/battle_mips_adapter.c -o "$TEST_OUT/$opt.cpu.o"
 gcc -std=gnu17 -fno-pie -fdata-sections "${flags[@]}" -c "$TEST_OUT/generated-stub.c" -o "$TEST_OUT/$opt.stub.o"
 for consumer in production separate-stub wrong-offset-plus1 wrong-offset-plus3; do
  gcc -std=gnu17 -fno-pie -ffunction-sections -fdata-sections "${flags[@]}" -DXENO_PC_PORT \
   -Ipc_port/include_shim -Ipc_port/src -Wall -Wextra -Werror -c "$TEST_OUT/consumer-$consumer.c" \
   -o "$TEST_OUT/$opt.$consumer.o"
  clang -no-pie "${flags[@]}" -Wl,--gc-sections -Wl,--export-dynamic "$TEST_OUT/$opt.test.o" \
   "$TEST_OUT/$opt.cpu.o" "$TEST_OUT/$opt.$consumer.o" "$TEST_OUT/$opt.stub.o" -ldl \
   -o "$TEST_OUT/$opt.$consumer.test"
  set +e; "$TEST_OUT/$opt.$consumer.test" "$TEST_OUT/map2-selector1-record.bin" \
   > "$TEST_OUT/$opt.$consumer.log" 2>&1; status=$?; set -e
  if [ "$consumer" = production ]; then
   cat "$TEST_OUT/$opt.$consumer.log"; if [ "$status" -ne 0 ]; then failed=1; fi
  elif [ "$status" -eq 0 ] || ! rg -q 'BATTLE ENCOUNTER ALIAS RED' "$TEST_OUT/$opt.$consumer.log"; then
   cat "$TEST_OUT/$opt.$consumer.log" >&2; echo "BATTLE ENCOUNTER ALIAS FAIL mutant survived: $consumer ($opt)" >&2; failed=1
  else echo "BATTLE ENCOUNTER ALIAS negative control PASS: $consumer ($opt)"; fi
 done
done
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json,os
pins=json.loads((Path(os.environ['TEST_OUT'])/'provenance.json').read_text())
for name,expected in pins.items():
 if '/' in name and name.startswith(('pc_port/','src/','config/','linker/','tools/')):
  assert sha256(Path(name).read_bytes()).hexdigest()==expected,'Source changed during test: '+name
PY
if [ "$failed" -ne 0 ]; then echo "BATTLE ENCOUNTER ALIAS FAIL: see $TEST_OUT" >&2; exit 1; fi
echo "BATTLE ENCOUNTER ALIAS PASS O0/O2/UBSan with 9 rejected mutant configurations; see $TEST_OUT"
