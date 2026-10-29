#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_TARGET_LIST_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-target-list-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
# Type/fault diagnostics need no external symbolizer or debuginfo download.
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-symbolize=0}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE TARGET LIST artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1]);source_module=pathlib.Path('scratchpad/native-module-20261017/module-test-2/module.bin')
identities={str(source_module):'4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885',
 'disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'}
for path,digest in identities.items():assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()==digest,path
(out/'module.bin').write_bytes(source_module.read_bytes())
source=pathlib.Path('src/battle/main41.c').read_text()
a=source.index('u16 func_80089C08(u8 index) {');b=source.index('\n}',a)+2;mask=source[a:b]
(out/'mask.c').write_text('#include <stdint.h>\n#include "psx_memory.h"\ntypedef uint16_t u16;typedef uint8_t u8;\n#include "battle_overlay_guest_ram.h"\n'+mask+'\n')
paths=list(map(str,pathlib.Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
 'pc_port/src/controller_vblank_service.c','pc_port/src/battle_overlay_guest_ram.h','src/battle/main41.c',
 'pc_port/tests/battle_module_stage1_host_test.c','pc_port/tests/battle_target_list_host_test.c',
 'pc_port/tests/run_battle_target_list_host_test.sh','asm/battle/nonmatchings/mainasm_146F0/func_800841E0.s',
 'asm/battle/matchings/main35_p2/func_80083FF4.s','asm/battle/matchings/main35_p4/func_80085310.s']
(out/'provenance.json').write_text(json.dumps({'scope':'Actual target list, eligibility and priority routines, including complete command initialization; no modeled game callees. Full production bus, frames and refusal boundaries.',
 'private_inputs':identities,'mask_body_sha256':hashlib.sha256(mask.encode()).hexdigest(),
 'source_pins':{p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in paths}},indent=2)+'\n')
PYCODE
python3 tools/scripts/gen_battle_bridge_map.py --symbols config/symbol_addrs.slus_006.64.txt \
 --symbols linker/undefined_funcs_auto.battle.txt --symbols linker/undefined_syms_auto.battle.txt \
 --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"
common=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -include assert.h -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan;do
 compiler=gcc;flags=(-"$mode")
 if [ "$mode" = UBSan ];then compiler=clang;flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
 for spec in test:pc_port/tests/battle_target_list_host_test.c cpu:pc_port/src/battle_mips_adapter.c \
             vblank:pc_port/src/controller_vblank_service.c mask:"$OUT/mask.c";do
  "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "${spec#*:}" -o "$OUT/$mode.${spec%%:*}.o"
 done
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.test.o" \
  "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" "$OUT/$mode.mask.o" -ldl -lcrypto -o "$OUT/$mode.test"
 if ! "$OUT/$mode.test" "$OUT/module.bin" > "$OUT/$mode.log" 2>&1;then cat "$OUT/$mode.log" >&2;exit 1;fi
 rg '^BATTLE TARGET LIST PASS 1056' "$OUT/$mode.log"
done
python3 - "$OUT" <<'PYCODE'
import pathlib,sys
out=pathlib.Path(sys.argv[1]);runtime=pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body=pathlib.Path('pc_port/src/battle_target_list_host.inc').read_text()
mutations={
 'ignore-death-bit':('0xc001u','0xc000u'),
 'wrong-alternate-stride':('0x800d32a1u+target*8u','0x800d32a1u+target*4u'),
 'wrong-candidate-frame':('sp+0x10u','sp+0x14u'),
 'wrong-group-partition':('TL_V1==TL_R8(0x800c3eb4u+target_offset)','TL_V1!=TL_R8(0x800c3eb4u+target_offset)'),
 'signed-rank-compare':('if (rank<TL_V1)','if ((int16_t)rank<(int16_t)TL_V1)'),
 'signed-priority':('TL_R16(0x800c3ebeu+target*28u)<TL_V1','(int16_t)TL_R16(0x800c3ebeu+target*28u)<(int16_t)TL_V1'),
 'wrong-eligibility-callsite':('party ? 0x80084260u : 0x800842bcu','party ? 0x80084264u : 0x800842bcu'),
 'lost-saved-s4':('TL_S4=TL_R32(sp+0x48u)','TL_S4=0'),
 'unguarded-identity':('expected[i*2+1]) return 0;','expected[i*2+1]) break;')}
for name,(old,new) in mutations.items():
 assert old in body,name
 p=out/(name+'.inc');p.write_text(body.replace(old,new))
 (out/(name+'.c')).write_text(runtime.replace('#include "battle_target_list_host.inc"','#include "'+str(p)+'"'))
PYCODE
for mutant in ignore-death-bit wrong-alternate-stride wrong-candidate-frame wrong-group-partition signed-rank-compare signed-priority wrong-eligibility-callsite lost-saved-s4 unguarded-identity;do
 gcc "${common[@]}" -O2 -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
  -c pc_port/tests/battle_target_list_host_test.c -o "$OUT/$mutant.o"
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" "$OUT/O2.cpu.o" \
  "$OUT/O2.vblank.o" "$OUT/O2.mask.o" -ldl -lcrypto -o "$OUT/$mutant.test"
 status=0;"$OUT/$mutant.test" "$OUT/module.bin" > "$OUT/$mutant.log" 2>&1 || status=$?
 if [ "$status" != 1 ] || ! rg -q '^BATTLE MODULE STAGE1 FAIL ' "$OUT/$mutant.log";then
  cat "$OUT/$mutant.log" >&2;printf 'Unexpected control %s status=%s\n' "$mutant" "$status" >&2;exit 1
 fi
 printf 'BATTLE TARGET LIST negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():
 assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE TARGET LIST source pins unchanged')
PYCODE
