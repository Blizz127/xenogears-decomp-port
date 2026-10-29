#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_MODULE_STAGE2_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-stage2-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE MODULE STAGE2 artifacts=%s\n' "$OUT"
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
 'pc_port/tests/battle_module_stage2_host_test.c','pc_port/tests/battle_module_stage1_host_test.c',
 'pc_port/tests/run_battle_module_stage2_host_test.sh']
(out/'provenance.json').write_text(json.dumps({'scope':'Original stage-two inventory, turn order and command callers vs native bodies, separately and through dispatcher. Explicit modeled game callees; no complete stage-two or battle acceptance.',
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
 for spec in test:pc_port/tests/battle_module_stage2_host_test.c cpu:pc_port/src/battle_mips_adapter.c \
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
body=pathlib.Path('pc_port/src/battle_module_stage2_host.inc').read_text()
mutations={'wrong-quantity-cap': ('M2_W8(0x8006f5c4u+i,99)', 'M2_W8(0x8006f5c4u+i,100)'), 'keep-empty-items': ('if (M2_R8(0x8006f5c4u+i)==0)', 'if (M2_R8(0x8006f5c4u+i)==255)'), 'wrong-primary-bound': ('i<150 && count<48', 'i<150 && count<47'), 'cap-secondary-list': ('i<100;i++', 'i<100 && count<48;i++'), 'wide-quantity-store': ('M2_W8(0x800d2cb0u+count,M2_R8(0x8006f5c4u+i))', 'M2_W16(0x800d2cb0u+count,M2_R8(0x8006f5c4u+i))'), 'wrong-random-range': ('0x801e4eb0u,0,10', '0x801e4eb0u,0,9'), 'wrong-order-count': ('(int32_t)M2_S0<11', '(int32_t)M2_S0<10'), 'unsigned-turn-minimum': ('int32_t value=(int16_t)M2_R16', 'int32_t value=(uint16_t)M2_R16'), 'wrong-special-character': ('id==M2_S7 ?', 'id==4 ?'), 'stale-menu-pointer': ('M2_W8(M2_R32(0x800c3eacu)+M2_S1+0x3cu,M2_V0)', 'M2_W8(0x80120000u+M2_S1+0x3cu,M2_V0)'), 'wrong-command-mask': ('bits&mask', 'bits|mask'), 'lost-frame-pointer': ('M2_FP=M2_R32(sp+0x30u)', 'M2_FP=0'), 'unguarded-identity': ('expected[i*2+1]) return 0;', 'expected[i*2+1]) break;')}
for name,(old,new) in mutations.items():
 assert old in body,name
 p=out/(name+'.inc');p.write_text(body.replace(old,new))
 (out/(name+'.c')).write_text(runtime.replace('#include "battle_module_stage2_host.inc"','#include "'+str(p)+'"'))
PYCODE
for mutant in wrong-quantity-cap keep-empty-items wrong-primary-bound cap-secondary-list wide-quantity-store wrong-random-range wrong-order-count unsigned-turn-minimum wrong-special-character stale-menu-pointer wrong-command-mask lost-frame-pointer unguarded-identity;do
 gcc "${common[@]}" -O2 -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
  -c pc_port/tests/battle_module_stage2_host_test.c -o "$OUT/$mutant.o"
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" "$OUT/O2.cpu.o" \
  "$OUT/O2.vblank.o" "$OUT/O2.mask.o" -ldl -lcrypto -o "$OUT/$mutant.test"
 status=0;"$OUT/$mutant.test" "$OUT/module.bin" > "$OUT/$mutant.log" 2>&1 || status=$?
 if [ "$status" != 1 ] || ! rg -q '^BATTLE MODULE STAGE1 FAIL ' "$OUT/$mutant.log";then
  cat "$OUT/$mutant.log" >&2;printf 'Unexpected control %s status=%s\n' "$mutant" "$status" >&2;exit 1
 fi
 printf 'BATTLE MODULE STAGE2 negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():
 assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE MODULE STAGE2 source pins unchanged')
PYCODE
