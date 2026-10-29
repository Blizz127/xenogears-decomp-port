#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_TURN_SPEED_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-turn-speed-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE TURN SPEED artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
source_module=pathlib.Path('scratchpad/native-module-20261017/module-test-2/module.bin')
identities={str(source_module):'4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885',
 'disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291',
 'disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'}
for path,digest in identities.items():
 assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()==digest,path
(out/'module.bin').write_bytes(source_module.read_bytes())
source=pathlib.Path('src/battle/main41.c').read_text()
a=source.index('u16 func_80089C08(u8 index) {');b=source.index('\n}',a)+2
mask=source[a:b]
(out/'mask.c').write_text('#include <stdint.h>\n#include "psx_memory.h"\ntypedef uint16_t u16;typedef uint8_t u8;\n#include "battle_overlay_guest_ram.h"\n'+mask+'\n')
source=pathlib.Path('src/slus_006.64/system/temp3.c').read_text()
a=source.index('u8 func_8001BD40(u8 min, u8 max) {');b=source.index('\n}',a)+2
bounded=source[a:b]
(out/'range.c').write_text('#include <stdint.h>\n#include <stdlib.h>\ntypedef uint8_t u8;\n#define REG_PIN(type,name,reg) type name\n'+bounded+'\n')
paths=list(map(str,pathlib.Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
 'pc_port/src/controller_vblank_service.c','pc_port/src/battle_overlay_guest_ram.h','src/battle/main41.c',
 'src/slus_006.64/system/temp3.c','pc_port/tests/battle_module_stage1_host_test.c',
 'pc_port/tests/battle_turn_speed_host_test.c','pc_port/tests/run_battle_turn_speed_host_test.sh',
 'asm/battle/nonmatchings/main8/func_80078508.s','asm/battle/nonmatchings/main53/func_80098AF8.s',
 'asm/battle/nonmatchings/main51/func_80094D24.s',
 'asm/slus_006.64/matchings/system/temp3c/func_8001BD40.s','asm/slus_006.64/matchings/psyq/libc/rand.s']
(out/'provenance.json').write_text(json.dumps({'scope':'Original reset, speed and flag game bodies versus native, including original turn-order caller and actual native RNG services. RNG service contract compares return and seed, not original CPU scratch registers or frame.',
 'private_inputs':identities,'mask_body_sha256':hashlib.sha256(mask.encode()).hexdigest(),
 'range_body_sha256':hashlib.sha256(bounded.encode()).hexdigest(),
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
 for spec in test:pc_port/tests/battle_turn_speed_host_test.c cpu:pc_port/src/battle_mips_adapter.c \
             vblank:pc_port/src/controller_vblank_service.c mask:"$OUT/mask.c" range:"$OUT/range.c"; do
  "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "${spec#*:}" -o "$OUT/$mode.${spec%%:*}.o"
 done
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.test.o" \
  "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" "$OUT/$mode.mask.o" "$OUT/$mode.range.o" -ldl -lcrypto -o "$OUT/$mode.test"
 if ! "$OUT/$mode.test" "$OUT/module.bin" > "$OUT/$mode.log" 2>&1;then cat "$OUT/$mode.log" >&2;exit 1;fi
 tail -n 1 "$OUT/$mode.log"
done
python3 - "$OUT" <<'PYCODE'
import pathlib,sys
out=pathlib.Path(sys.argv[1]);runtime=pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body=pathlib.Path('pc_port/src/battle_turn_speed_host.inc').read_text()
mutations={
 'inactive-zero':('TS_S5=0xff','TS_S5=0'),
 'wrong-output-stride':('TS_S4++;','TS_S4+=2;'),
 'lost-reset-frame':('TS_W32(sp+0x10u+r*4u,entry->cpu->gpr[16+r])','TS_W32(sp+0x10u+r*4u,0)'),
 'wide-slot':('entry->cpu->gpr[4]&0xffu','entry->cpu->gpr[4]&0xffffu'),
 'wrong-gear-stride':('slot*0x690u','slot*0x5f0u'),
 'unsigned-penalty':('(int8_t)TS_R8(table+0x27u)','(uint8_t)TS_R8(table+0x27u)'),
 'wrong-speed-cap':('TS_S0=0xa0','TS_S0=0xa5'),
 'unsigned-spread':('(int32_t)TS_V0%8-4','(int32_t)(TS_V0%8)-4'),
 'wrong-flag-clear':('last_flags&0xfffdu','last_flags&0xfffeu'),
 'narrow-restore':('TS_W16(record+0x7au,','TS_W8(record+0x7au,'),
 'wrong-flags-return':('TS_V0=3; TS_V1=last_flags','TS_V0=2; TS_V1=last_flags'),
 'unguarded-identity':('expected[i*2+1]) return 0;','expected[i*2+1]) break;')}
for name,(old,new) in mutations.items():
 assert old in body,name
 p=out/(name+'.inc');p.write_text(body.replace(old,new))
 (out/(name+'.c')).write_text(runtime.replace('#include "battle_turn_speed_host.inc"','#include "'+str(p)+'"'))
bounded=(out/'range.c').read_text();old=': (rr & 0xFF);';assert old in bounded
(out/'zero-remainder.c').write_text(bounded.replace(old,': 0;'))
old='uint8_t (*random_range)(uint8_t,uint8_t) =\n            (uint8_t (*)(uint8_t,uint8_t))resolved->host;'
assert old in runtime
(out/'wrong-range-abi.c').write_text(runtime.replace(old,'uint32_t (*random_range)(uint32_t, uint32_t) = (uint32_t (*)(uint32_t, uint32_t))resolved->host;'))
PYCODE
for mutant in inactive-zero wrong-output-stride lost-reset-frame wide-slot wrong-gear-stride unsigned-penalty wrong-speed-cap unsigned-spread wrong-flag-clear narrow-restore wrong-flags-return unguarded-identity;do
 gcc "${common[@]}" -O2 -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
  -c pc_port/tests/battle_turn_speed_host_test.c -o "$OUT/$mutant.o"
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" "$OUT/O2.cpu.o" \
  "$OUT/O2.vblank.o" "$OUT/O2.mask.o" "$OUT/O2.range.o" -ldl -lcrypto -o "$OUT/$mutant.test"
 status=0;"$OUT/$mutant.test" "$OUT/module.bin" > "$OUT/$mutant.log" 2>&1 || status=$?
 if [ "$status" != 1 ] || ! rg -q '^BATTLE MODULE STAGE1 FAIL ' "$OUT/$mutant.log";then
  cat "$OUT/$mutant.log" >&2;printf 'Unexpected control %s status=%s\n' "$mutant" "$status" >&2;exit 1
 fi
 printf 'BATTLE TURN SPEED negative control rejected: %s\n' "$mutant"
done
gcc "${common[@]}" -O2 -Wall -Wextra -Werror -c "$OUT/zero-remainder.c" -o "$OUT/zero-remainder.o"
clang -no-pie -O2 -Wl,--gc-sections "$OUT/O2.test.o" "$OUT/O2.compat.o" "$OUT/O2.cpu.o" \
 "$OUT/O2.vblank.o" "$OUT/O2.mask.o" "$OUT/zero-remainder.o" -ldl -lcrypto -o "$OUT/zero-remainder.test"
status=0;"$OUT/zero-remainder.test" "$OUT/module.bin" --range-only > "$OUT/zero-remainder.log" 2>&1 || status=$?
if [ "$status" != 1 ] || ! rg -q 'native RNG service return/state' "$OUT/zero-remainder.log";then cat "$OUT/zero-remainder.log" >&2;exit 1;fi
printf 'BATTLE TURN SPEED negative control rejected: zero-remainder\n'
clang "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all -Wall -Wextra -Werror \
 "-DBATTLE_RUNTIME_SOURCE=\"$OUT/wrong-range-abi.c\"" -c pc_port/tests/battle_turn_speed_host_test.c -o "$OUT/wrong-range-abi.o"
clang -no-pie -O1 -fsanitize=undefined -fno-sanitize-recover=all -Wl,--gc-sections \
 "$OUT/wrong-range-abi.o" "$OUT/UBSan.compat.o" "$OUT/UBSan.cpu.o" "$OUT/UBSan.vblank.o" \
 "$OUT/UBSan.mask.o" "$OUT/UBSan.range.o" -ldl -lcrypto -o "$OUT/wrong-range-abi.test"
status=0;"$OUT/wrong-range-abi.test" "$OUT/module.bin" --range-only > "$OUT/wrong-range-abi.log" 2>&1 || status=$?
if [ "$status" != 1 ] || ! rg -q 'call to function .* through pointer to incorrect function type' "$OUT/wrong-range-abi.log";then cat "$OUT/wrong-range-abi.log" >&2;exit 1;fi
printf 'BATTLE TURN SPEED negative control rejected: wrong-range-abi\n'
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():
 assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE TURN SPEED source pins unchanged')
PYCODE
