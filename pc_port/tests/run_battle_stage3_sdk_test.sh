#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_STAGE3_SDK_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-stage3-sdk.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-symbolize=0}
mkdir -p "$TMPDIR"
ubsan_preflight clang
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1]);image=pathlib.Path('disc/SLUS_006.64')
digest=hashlib.sha256(image.read_bytes()).hexdigest()
assert digest=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
source=pathlib.Path('src/battle/main41.c').read_text()
a=source.index('u16 func_80089C08(u8 index) {');b=source.index('\n}',a)+2
(out/'mask.c').write_text('#include <stdint.h>\n#include "psx_memory.h"\ntypedef uint16_t u16;typedef uint8_t u8;\n#include "battle_overlay_guest_ram.h"\n'+source[a:b]+'\n')
paths=list(map(str,pathlib.Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
 'pc_port/src/controller_vblank_service.c','pc_port/src/battle_overlay_guest_ram.h','src/battle/main41.c',
 'pc_port/src/retail_leaf_adapters.c','pc_port/src/psyq_compat_leaf.inc',
 'pc_port/extern/PsyCross/src/psx/LIBGPU.C','pc_port/extern/PsyCross/include/psx/libgpu.h',
 'pc_port/tests/battle_module_stage1_host_test.c','pc_port/tests/battle_stage3_sdk_test.c',
 'pc_port/tests/run_battle_stage3_sdk_test.sh']
(out/'provenance.json').write_text(json.dumps(dict(scope='Actual SDK/compat packet setters and native GetClut arithmetic against original SDK code. Graph type 0, packet bytes and scalar return contract only; no service CPU-frame/volatile-state or hardware parity.',
 private_inputs={str(image):digest},source_pins={p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in paths}),indent=2)+'\n')
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
 compiler=gcc;flags=(-"$mode");extra=(-fpermissive)
 if [ "$mode" = UBSan ];then compiler=clang;flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);extra=();fi
 clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
 "$compiler" "${common[@]}" "${flags[@]}" "${extra[@]}" -w -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$mode.leaves.o"
 for spec in test:pc_port/tests/battle_stage3_sdk_test.c cpu:pc_port/src/battle_mips_adapter.c \
             vblank:pc_port/src/controller_vblank_service.c mask:"$OUT/mask.c";do
  "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "${spec#*:}" -o "$OUT/$mode.${spec%%:*}.o"
 done
 g++ "${sdk_common[@]}" "${sdl_flags[@]}" "${flags[@]}" -c pc_port/extern/PsyCross/src/psx/LIBGPU.C -o "$OUT/$mode.gpu.o"
 clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.test.o" "$OUT/$mode.compat.o" "$OUT/$mode.leaves.o" \
  "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" "$OUT/$mode.mask.o" "$OUT/$mode.gpu.o" -ldl -lcrypto -o "$OUT/$mode.test"
 if ! "$OUT/$mode.test" > "$OUT/$mode.log" 2>&1;then cat "$OUT/$mode.log" >&2;exit 1;fi
 rg '^BATTLE STAGE3 SDK PASS ' "$OUT/$mode.log"
done
python3 - "$OUT" <<'PYCODE'
import pathlib,sys
out=pathlib.Path(sys.argv[1]);runtime=pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
old='cpu->gpr[5]<<6';assert runtime.count(old)==1
(out/'wrong-clut-shift.c').write_text(runtime.replace(old,'cpu->gpr[5]<<5'))
PYCODE
gcc "${common[@]}" -O2 -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/wrong-clut-shift.c\"" \
 -c pc_port/tests/battle_stage3_sdk_test.c -o "$OUT/wrong-clut-shift.o"
clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/wrong-clut-shift.o" "$OUT/O2.compat.o" "$OUT/O2.leaves.o" \
 "$OUT/O2.cpu.o" "$OUT/O2.vblank.o" "$OUT/O2.mask.o" "$OUT/O2.gpu.o" -ldl -lcrypto -o "$OUT/wrong-clut-shift.test"
status=0;"$OUT/wrong-clut-shift.test" > "$OUT/wrong-clut-shift.log" 2>&1 || status=$?
if [ "$status" != 1 ] || ! rg -q 'stage3 SDK original packet/return' "$OUT/wrong-clut-shift.log";then cat "$OUT/wrong-clut-shift.log" >&2;exit 1;fi
printf 'BATTLE STAGE3 SDK negative control rejected: wrong-clut-shift\n'
python3 - "$OUT" <<'PYCODE'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE STAGE3 SDK source pins unchanged')
PYCODE
