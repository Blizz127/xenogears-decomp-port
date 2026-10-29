#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_PARTY_STATS_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-party-stats-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE PARTY STATS artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
source_module=pathlib.Path('scratchpad/native-module-20261017/module-test-2/module.bin')
module=source_module.read_bytes()
assert hashlib.sha256(module).hexdigest()=='4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885'
(out/'module.bin').write_bytes(module)
battle=pathlib.Path('disc/battle.bin').read_bytes()
assert hashlib.sha256(battle).hexdigest()=='1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'

source=pathlib.Path('src/battle/main41.c').read_text()
a=source.index('u16 func_80089C08(u8 index) {');b=source.index('\n}',a)+2
helper=source[a:b]
(out/'mask.c').write_text('#include <stdint.h>\n#include "psx_memory.h"\ntypedef uint16_t u16;typedef uint8_t u8;\n#include "battle_overlay_guest_ram.h"\n'+helper+'\n')
paths=list(map(str,pathlib.Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
 'pc_port/src/controller_vblank_service.c','pc_port/src/battle_overlay_guest_ram.h','src/battle/main41.c',
 'pc_port/tests/battle_party_stats_host_test.c','pc_port/tests/battle_module_stage1_host_test.c',
 'src/battle/main53.c','src/battle/main57.c','asm/battle/nonmatchings/main53/func_80097D5C.s','asm/battle/matchings/main57/func_8009B098.s','pc_port/tests/run_battle_party_stats_host_test.sh']
(out/'provenance.json').write_text(json.dumps({'scope':'Original party-stat and special-mode leaves vs native bodies, plus actor-mode and full stage-1 composition with actual libc and mask leaf. No modeled callees in these cases; no complete battle acceptance.',
 'battle_sha256':hashlib.sha256(battle).hexdigest(),'module_source':str(source_module),'module_sha256':hashlib.sha256(module).hexdigest(),
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
 for spec in test:pc_port/tests/battle_party_stats_host_test.c cpu:pc_port/src/battle_mips_adapter.c \
             vblank:pc_port/src/controller_vblank_service.c mask:"$OUT/mask.c"; do
  "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "${spec#*:}" -o "$OUT/$mode.${spec%%:*}.o"
 done
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.test.o" \
  "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" "$OUT/$mode.mask.o" -ldl -lcrypto -o "$OUT/$mode.test"
 if ! "$OUT/$mode.test" "$OUT/module.bin" > "$OUT/$mode.log" 2>&1;then cat "$OUT/$mode.log" >&2;exit 1;fi
 tail -n 1 "$OUT/$mode.log"
done
python3 - "$OUT" <<'PYCODE'
import pathlib,sys
out=pathlib.Path(sys.argv[1]);runtime=pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body=pathlib.Path('pc_port/src/battle_party_stats_host.inc').read_text()
mutations={
 'wrong-empty-slot':('PS_R8(0x800d2d24u+slot)==0x7fu','PS_R8(0x800d2d24u+slot)==0xffu'),
 'wrong-percentage':('PS_R8(record+0x30u)/20u','PS_R8(record+0x30u)/19u'),
 'wide-hp-store':('PS_W16(record+0x4cu,PS_R16(record+0x4cu)+hp_delta);','PS_W32(record+0x4cu,PS_R16(record+0x4cu)+hp_delta);'),
 'wide-gear-product':('uint32_t scaled=(PS_R32(stats+0x64u)/10u)*gear_bonus*2u;','uint64_t scaled=((uint64_t)PS_R32(stats+0x64u)/10u)*gear_bonus*2u;'),
 'lost-halfword-truncation':('uint32_t truncated=PS_R16(row);','uint32_t truncated=scaled/9u;'),
 'floor-effect-cost':('(cost+1u)/2u','cost/2u'),
 'wrong-actor-seven':('PS_R8(record+0x56u)==7','PS_R8(record+0x56u)==4'),
 'wrong-special-stride':('base+i*0x170u','base+i*0x16cu'),
 'lost-hilo':('entry->cpu->hi=(uint32_t)(last_product>>32);','entry->cpu->hi=0;'),
 'unguarded-identity':('identity[i*2+1]) return 0;','identity[i*2+1]) break;')}
for name,(old,new) in mutations.items():
 assert old in body,name
 p=out/(name+'.inc');p.write_text(body.replace(old,new))
 (out/(name+'.c')).write_text(runtime.replace('#include "battle_party_stats_host.inc"','#include "'+str(p)+'"'))
PYCODE
for mutant in wrong-empty-slot wrong-percentage wide-hp-store wide-gear-product lost-halfword-truncation \
              floor-effect-cost wrong-actor-seven wrong-special-stride lost-hilo unguarded-identity;do
 gcc "${common[@]}" -O2 -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
  -c pc_port/tests/battle_party_stats_host_test.c -o "$OUT/$mutant.o"
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" "$OUT/O2.cpu.o" \
  "$OUT/O2.vblank.o" "$OUT/O2.mask.o" -ldl -lcrypto -o "$OUT/$mutant.test"
 status=0;"$OUT/$mutant.test" "$OUT/module.bin" > "$OUT/$mutant.log" 2>&1 || status=$?
 if [ "$status" != 1 ] || ! rg -q '^BATTLE MODULE STAGE1 FAIL ' "$OUT/$mutant.log";then
  cat "$OUT/$mutant.log" >&2;printf 'Unexpected control %s status=%s\n' "$mutant" "$status" >&2;exit 1
 fi
 printf 'BATTLE PARTY STATS negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():
 assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE PARTY STATS source pins unchanged')
PYCODE
