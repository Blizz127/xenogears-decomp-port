#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_MODULE_STAGE1_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-stage1-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE MODULE STAGE1 artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
source_module=pathlib.Path('scratchpad/native-module-20261017/module-test-2/module.bin')
module=source_module.read_bytes()
assert hashlib.sha256(module).hexdigest()=='4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885'
(out/'module.bin').write_bytes(module)
source=pathlib.Path('src/battle/main41.c').read_text()
a=source.index('u16 func_80089C08(u8 index) {');b=source.index('\n}',a)+2
helper=source[a:b]
(out/'mask.c').write_text('#include <stdint.h>\n#include "psx_memory.h"\ntypedef uint16_t u16;typedef uint8_t u8;\n#include "battle_overlay_guest_ram.h"\n'+helper+'\n')
paths=list(map(str,pathlib.Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
 'pc_port/src/controller_vblank_service.c','pc_port/src/battle_overlay_guest_ram.h','src/battle/main41.c',
 'pc_port/tests/battle_module_stage1_host_test.c','pc_port/tests/run_battle_module_stage1_host_test.sh']
mask_code=pathlib.Path('disc/battle.bin').read_bytes()[0x1a118:0x1a134]
mask_code_sha=hashlib.sha256(mask_code).hexdigest()
assert mask_code_sha=='74c04258a3cf89081e9e82cef4ed802aff2c03b830a9fc7bb204291532cb45c0'
(out/'provenance.json').write_text(json.dumps({'scope':'Original stage-1 callers versus native C; modeled 97D5C/9B098. Separate libc and canonical native mask-body integration. No complete battle acceptance.',
 'mask_code_source':'disc/battle.bin','mask_code_offset':0x1a118,'mask_code_sha256':mask_code_sha,
 'module_source':str(source_module),'module_sha256':hashlib.sha256(module).hexdigest(),
 'mask_body_sha256':hashlib.sha256(helper.encode()).hexdigest(),
 'source_pins':{p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in paths}},indent=2)+'\n')
PYCODE
python3 tools/scripts/gen_battle_bridge_map.py --symbols config/symbol_addrs.slus_006.64.txt \
 --symbols linker/undefined_funcs_auto.battle.txt --symbols linker/undefined_syms_auto.battle.txt \
 --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"
common=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -include assert.h -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 compiler=gcc;flags=(-"$mode")
 if [ "$mode" = UBSan ]; then compiler=clang;flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
 for spec in test:pc_port/tests/battle_module_stage1_host_test.c cpu:pc_port/src/battle_mips_adapter.c \
             vblank:pc_port/src/controller_vblank_service.c mask:"$OUT/mask.c"; do
  "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "${spec#*:}" -o "$OUT/$mode.${spec%%:*}.o"
 done
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.test.o" \
  "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" "$OUT/$mode.mask.o" -ldl -lcrypto -o "$OUT/$mode.test"
 for suite in model sdk; do
  args=();if [ "$suite" = sdk ];then args=(--sdk);fi
  if ! "$OUT/$mode.test" "$OUT/module.bin" "${args[@]}" > "$OUT/$mode.$suite.log" 2>&1;then cat "$OUT/$mode.$suite.log" >&2;exit 1;fi
  tail -n 1 "$OUT/$mode.$suite.log"
 done
done
python3 - "$OUT" <<'PYCODE'
import pathlib,sys
out=pathlib.Path(sys.argv[1]);runtime=pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body=pathlib.Path('pc_port/src/battle_module_stage1_host.inc').read_text()
mutations={
 'wrong-presence-mask':('(M1_R8(0x800c3eb6u+i*0x1cu)&0x7fu)','M1_R8(0x800c3eb6u+i*0x1cu)'),
 'wrong-enemy-group':('(side ? 8u : 0u)','(side ? 0u : 0u)'),
 'wrong-position-stride':('M1_R8(record)<<5','M1_R8(record)<<4'),
 'wrong-record-destination':('0x801e4934u,0x800ccce8u+M1_S4','0x801e4934u,0x800cccecu+M1_S4'),
 'wrong-resource-sentinel':('offset != 0xffffu','offset != 0xfffeu'),
 'stale-scenario':('scenario=M1_R32(0x800c3dd0u);\n            uint32_t header','scenario=M1_S7-0x32u;\n            uint32_t header'),
 'wrong-actor-exception':('M1_R8(record+2u) != 7','M1_R8(record+2u) != 6'),
 'one-region-gate':('region<2','region<1')}
for name,(old,new) in mutations.items():
 assert old in body,name
 p=out/(name+'.inc');p.write_text(body.replace(old,new))
 (out/(name+'.c')).write_text(runtime.replace('#include "battle_module_stage1_host.inc"','#include "'+str(p)+'"'))
anchor='cpu->gpr[2] = cpu->gpr[4];\n        cpu->gpr[3] = 0;\n    } else if (strcmp(resolved->name, "bzero")'
assert runtime.count(anchor)==1
(out/'wrong-pointer-return.c').write_text(runtime.replace(anchor,anchor.replace('= cpu->gpr[4]','= cpu->gpr[4] + 4u')))
PYCODE
for mutant in wrong-presence-mask wrong-enemy-group wrong-position-stride wrong-record-destination \
              wrong-resource-sentinel stale-scenario wrong-actor-exception one-region-gate wrong-pointer-return;do
 gcc "${common[@]}" -O2 -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
  -c pc_port/tests/battle_module_stage1_host_test.c -o "$OUT/$mutant.o"
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" "$OUT/O2.cpu.o" \
  "$OUT/O2.vblank.o" "$OUT/O2.mask.o" -ldl -lcrypto -o "$OUT/$mutant.test"
 args=();if [ "$mutant" = wrong-pointer-return ];then args=(--sdk);fi
 status=0;"$OUT/$mutant.test" "$OUT/module.bin" "${args[@]}" > "$OUT/$mutant.log" 2>&1 || status=$?
 if [ "$status" != 1 ] || ! rg -q '^BATTLE MODULE STAGE1 FAIL ' "$OUT/$mutant.log";then
  cat "$OUT/$mutant.log" >&2;printf 'Unexpected control %s status=%s\n' "$mutant" "$status" >&2;exit 1
 fi
 printf 'BATTLE MODULE STAGE1 negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():
 assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE MODULE STAGE1 source pins unchanged')
PYCODE
