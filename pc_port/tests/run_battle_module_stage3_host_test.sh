#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_MODULE_STAGE3_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-stage3-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-symbolize=0}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE MODULE STAGE3 artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1]);module=pathlib.Path('scratchpad/native-module-20261017/module-test-2/module.bin')
identities={str(module):'4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885',
 'disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'}
for p,d in identities.items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
(out/'module.bin').write_bytes(module.read_bytes())
source=pathlib.Path('src/battle/main41.c').read_text()
a=source.index('u16 func_80089C08(u8 index) {');b=source.index('\n}',a)+2;mask=source[a:b]
(out/'mask.c').write_text('#include <stdint.h>\n#include "psx_memory.h"\ntypedef uint16_t u16;typedef uint8_t u8;\n#include "battle_overlay_guest_ram.h"\n'+mask+'\n')
paths=list(map(str,pathlib.Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
 'pc_port/src/controller_vblank_service.c','pc_port/src/battle_overlay_guest_ram.h','src/battle/main41.c',
 'src/slus_006.64/system/temp1f.c','pc_port/extern/PsyCross/src/psx/LIBGPU.C',
 'pc_port/src/retail_leaf_adapters.c','pc_port/src/psyq_compat_leaf.inc','pc_port/extern/PsyCross/include/psx/libgpu.h',
 'pc_port/tests/battle_module_stage1_host_test.c','pc_port/tests/battle_module_stage3_host_test.c',
 'pc_port/tests/run_battle_module_stage3_host_test.sh']
(out/'provenance.json').write_text(json.dumps(dict(scope='Original six stage-three callers plus dispatcher versus native. External game callees modeled; separate actual atlas/SDK and typed game-model integration. Original GetClut returns; not full game-service/rendering acceptance.',
 private_inputs=identities,source_pins={p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in paths}),indent=2)+'\n')
PYCODE
python3 tools/scripts/gen_battle_bridge_map.py --symbols config/symbol_addrs.slus_006.64.txt \
 --symbols linker/undefined_funcs_auto.battle.txt --symbols linker/undefined_syms_auto.battle.txt \
 --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"
common=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
 -include assert.h -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
 -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
sdk_common=(-std=c++17 -fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
 -fpermissive -w -include pc_port/src/port_compat.h -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"
for mode in O0 O2 UBSan;do
 compiler=gcc;flags=(-"$mode");atlas_flags=(-fpermissive)
 if [ "$mode" = UBSan ];then compiler=clang;flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);atlas_flags=();fi
 clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
 "$compiler" "${common[@]}" "${flags[@]}" "${atlas_flags[@]}" -w -c src/slus_006.64/system/temp1f.c -o "$OUT/$mode.atlas.o"
 "$compiler" "${common[@]}" "${flags[@]}" "${atlas_flags[@]}" -w -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$mode.leaves.o"
 for spec in test:pc_port/tests/battle_module_stage3_host_test.c cpu:pc_port/src/battle_mips_adapter.c \
             vblank:pc_port/src/controller_vblank_service.c mask:"$OUT/mask.c";do
  "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "${spec#*:}" -o "$OUT/$mode.${spec%%:*}.o"
 done
 g++ "${sdk_common[@]}" "${sdl_flags[@]}" "${flags[@]}" -c pc_port/extern/PsyCross/src/psx/LIBGPU.C -o "$OUT/$mode.gpu.o"
 clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.atlas.o" "$OUT/$mode.test.o" \
  "$OUT/$mode.cpu.o" "$OUT/$mode.leaves.o" "$OUT/$mode.vblank.o" "$OUT/$mode.mask.o" "$OUT/$mode.gpu.o" -ldl -lcrypto -o "$OUT/$mode.test"
 for suite in model sdk;do
  args=();if [ "$suite" = sdk ];then args=(--sdk);fi
  if ! "$OUT/$mode.test" "$OUT/module.bin" "${args[@]}" > "$OUT/$mode.$suite.log" 2>&1;then cat "$OUT/$mode.$suite.log" >&2;exit 1;fi
  rg '^BATTLE MODULE STAGE3 PASS ' "$OUT/$mode.$suite.log"
 done
done
python3 - "$OUT" <<'PYCODE'
import pathlib,sys
out=pathlib.Path(sys.argv[1]);runtime=pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body=pathlib.Path('pc_port/src/battle_module_stage3_host.inc').read_text()
mutations={
 'wrong-atlas-index':('table,0x5c,','table,0x5d,'),
 'short-primitive-loop':('(int32_t)M3_S2<8','(int32_t)M3_S2<7'),
 'wrong-glyph-skip':('!=0x7fu','!=0xffu'),
 'unsigned-coordinate':('(uint32_t)(int32_t)(int16_t)','(uint32_t)(uint16_t)'),
 'wrong-glyph-stride':('count*80u','count*84u'),
 'wrong-style-stride':('(M3_S0+slot)*40u','(M3_S0+slot)*80u'),
 'lost-frame-pointer':('M3_FP=M3_R32(sp+0x30u)','M3_FP=0'),
 'short-text-loop':('(int32_t)M3_S1<10','(int32_t)M3_S1<9'),
 'wrong-rectangle':('M3_W16(sp+0x1cu,0x100)','M3_W16(sp+0x1cu,0xff)'),
 'unguarded-identity':('expected[i*2+1]) return 0;','expected[i*2+1]) break;')}
for name,(old,new) in mutations.items():
 assert old in body,name
 p=out/(name+'.inc');p.write_text(body.replace(old,new))
 (out/(name+'.c')).write_text(runtime.replace('#include "battle_module_stage3_host.inc"','#include "'+str(p)+'"'))
old='((void (*)(DR_MODE *,int,int,int,RECT16 *))host)';assert old in runtime
(out/'wrong-draw-mode-abi.c').write_text(runtime.replace(old,'((void (*)(void *,int,int,int,void *))host)'))
PYCODE
for mutant in wrong-atlas-index short-primitive-loop wrong-glyph-skip unsigned-coordinate wrong-glyph-stride wrong-style-stride lost-frame-pointer short-text-loop wrong-rectangle unguarded-identity;do
 gcc "${common[@]}" -O2 -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
  -c pc_port/tests/battle_module_stage3_host_test.c -o "$OUT/$mutant.o"
 clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" "$OUT/O2.atlas.o" "$OUT/O2.cpu.o" \
  "$OUT/O2.leaves.o" "$OUT/O2.vblank.o" "$OUT/O2.mask.o" "$OUT/O2.gpu.o" -ldl -lcrypto -o "$OUT/$mutant.test"
 status=0;"$OUT/$mutant.test" "$OUT/module.bin" > "$OUT/$mutant.log" 2>&1 || status=$?
 if [ "$status" != 1 ] || ! rg -q '^BATTLE MODULE STAGE1 FAIL ' "$OUT/$mutant.log";then
  cat "$OUT/$mutant.log" >&2;printf 'Unexpected control %s status=%s\n' "$mutant" "$status" >&2;exit 1
 fi
 printf 'BATTLE MODULE STAGE3 negative control rejected: %s\n' "$mutant"
done
clang "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all -Wall -Wextra -Werror \
 "-DBATTLE_RUNTIME_SOURCE=\"$OUT/wrong-draw-mode-abi.c\"" -c pc_port/tests/battle_module_stage3_host_test.c -o "$OUT/wrong-draw-mode-abi.o"
clang++ -no-pie -O1 -fsanitize=undefined -fno-sanitize-recover=all -Wl,--gc-sections "$OUT/wrong-draw-mode-abi.o" \
 "$OUT/UBSan.compat.o" "$OUT/UBSan.atlas.o" "$OUT/UBSan.cpu.o" "$OUT/UBSan.leaves.o" "$OUT/UBSan.vblank.o" "$OUT/UBSan.mask.o" "$OUT/UBSan.gpu.o" -ldl -lcrypto -o "$OUT/wrong-draw-mode-abi.test"
status=0;"$OUT/wrong-draw-mode-abi.test" "$OUT/module.bin" --abi-only > "$OUT/wrong-draw-mode-abi.log" 2>&1 || status=$?
if [ "$status" != 1 ] || ! rg -q 'call to function .*incorrect function type' "$OUT/wrong-draw-mode-abi.log";then cat "$OUT/wrong-draw-mode-abi.log" >&2;exit 1;fi
printf 'BATTLE MODULE STAGE3 negative control rejected: wrong-draw-mode-abi\n'
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE MODULE STAGE3 source pins unchanged')
PYCODE
