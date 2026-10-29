#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_GLYPHS_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-glyphs-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-symbolize=0}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE GLYPHS artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1]);module=pathlib.Path('scratchpad/native-module-20261017/module-test-2/module.bin')
identities={str(module):'4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885',
 'disc/battle.bin':'1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291',
 'disc/SLUS_006.64':'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'}
for p,d in identities.items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
(out/'module.bin').write_bytes(module.read_bytes())
source=pathlib.Path('src/battle/main41.c').read_text()
a=source.index('u16 func_80089C08(u8 index) {');b=source.index('\n}',a)+2;mask=source[a:b]
(out/'mask.c').write_text('#include <stdint.h>\n#include "psx_memory.h"\ntypedef uint16_t u16;typedef uint8_t u8;\n#include "battle_overlay_guest_ram.h"\n'+mask+'\n')
(out/'style-declarations.h').write_text('#include <stdint.h>\nvoid func_80076AC8(uint8_t *);\n')
paths=list(map(str,pathlib.Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
 'pc_port/src/controller_vblank_service.c','pc_port/src/battle_overlay_guest_ram.h','src/battle/main41.c',
 'src/slus_006.64/system/temp1f.c','pc_port/extern/PsyCross/src/psx/LIBGPU.C',
 'pc_port/src/retail_data.h','src/battle/main4.c','src/battle/main5.c',
 'pc_port/src/retail_leaf_adapters.c','pc_port/src/psyq_compat_leaf.inc','pc_port/extern/PsyCross/include/psx/libgpu.h',
 'pc_port/tests/battle_module_stage1_host_test.c','pc_port/tests/battle_module_stage3_host_test.c',
 'pc_port/tests/battle_glyphs_host_test.c','pc_port/tests/run_battle_glyphs_host_test.sh']
(out/'provenance.json').write_text(json.dumps(dict(scope='Original glyph wrappers/renderer and actual stage-three callers versus native. Actual atlas, style, SDK services; SDK CPU frames/volatile state excluded. Not full battle or rendering acceptance.',
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
for mode in ${BATTLE_GLYPHS_TEST_MODES:-O0 O2 UBSan};do
 compiler=gcc;flags=(-"$mode");atlas_flags=(-fpermissive)
 if [ "$mode" = UBSan ];then compiler=clang;flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);atlas_flags=();fi
 clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
 "$compiler" "${common[@]}" "${flags[@]}" "${atlas_flags[@]}" -w -c src/slus_006.64/system/temp1f.c -o "$OUT/$mode.atlas.o"
 "$compiler" "${common[@]}" "${flags[@]}" "${atlas_flags[@]}" -w -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$mode.leaves.o"
 for source in main4 main5;do
  "$compiler" "${common[@]}" "${flags[@]}" "${atlas_flags[@]}" -DXENO_BATTLE_OVERLAY_HOST_BODIES -include "$OUT/style-declarations.h" -w \
   -c "src/battle/$source.c" -o "$OUT/$mode.$source.o"
 done
 for spec in test:pc_port/tests/battle_glyphs_host_test.c cpu:pc_port/src/battle_mips_adapter.c \
             vblank:pc_port/src/controller_vblank_service.c mask:"$OUT/mask.c";do
  "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "${spec#*:}" -o "$OUT/$mode.${spec%%:*}.o"
 done
 g++ "${sdk_common[@]}" "${sdl_flags[@]}" "${flags[@]}" -c pc_port/extern/PsyCross/src/psx/LIBGPU.C -o "$OUT/$mode.gpu.o"
 clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.atlas.o" "$OUT/$mode.test.o" \
  "$OUT/$mode.cpu.o" "$OUT/$mode.main4.o" "$OUT/$mode.main5.o" "$OUT/$mode.leaves.o" "$OUT/$mode.vblank.o" "$OUT/$mode.mask.o" "$OUT/$mode.gpu.o" -ldl -lcrypto -o "$OUT/$mode.test"
 args=();if [ -n "${BATTLE_GLYPHS_TEST_ENTRY:-}" ];then args=(--entry "$BATTLE_GLYPHS_TEST_ENTRY");fi
 if ! "$OUT/$mode.test" "$OUT/module.bin" "${args[@]}" > "$OUT/$mode.test.log" 2>&1;then cat "$OUT/$mode.test.log" >&2;exit 1;fi
 rg '^BATTLE GLYPHS (PASS|completed) ' "$OUT/$mode.test.log"
done
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE GLYPHS source pins unchanged')
PYCODE
