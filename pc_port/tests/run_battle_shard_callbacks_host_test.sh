#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_SHARD_CALLBACKS_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-shard-callbacks-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE SHARD CALLBACKS artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PY'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1]); battle=pathlib.Path('disc/battle.bin').read_bytes(); exe=pathlib.Path('disc/SLUS_006.64').read_bytes()
assert hashlib.sha256(battle).hexdigest()=='1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'
assert hashlib.sha256(exe).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
regions={}
for name,a,b,want in [('update',0x800b6f0c,0x800b7134,'40857615e2893015fee79945c85827fc7192c80d3010461d6e1c3210f64d711e'),
 ('render',0x800b7160,0x800b7330,'41e103d63547609e67d0796adaf1c9e9143e1c111132a62854c5feeed353aebf'),
 ('cleanup',0x800b7330,0x800b7364,'cbbfde0b27001af6425f36e435bd8cd5ba21eb4b27155157b70b11f39f6c90ed')]:
 digest=hashlib.sha256(battle[a-0x8006faf0:b-0x8006faf0]).hexdigest(); assert digest==want
 regions[name]={'start':hex(a),'end':hex(b),'sha256':digest}
for name,a,b in [('RotMatrix',0x8003f738,0x8003f8b0),('AddPrim',0x80043b48,0x80043b84),('trig_table',0x800523f0,0x800563f0)]:
 regions[name]={'start':hex(a),'end':hex(b),'sha256':hashlib.sha256(exe[a-0x8000f800:b-0x8000f800]).hexdigest()}
paths=list(map(str,pathlib.Path('pc_port/src').glob('battle_*_host.inc')))+[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
 'pc_port/src/retail_leaf_adapters.c','pc_port/src/psyq_compat_leaf.inc','pc_port/src/controller_vblank_service.c',
 'pc_port/include_shim/psyq/libgte.h','src/slus_006.64/system/animation_scripts.c',
 'pc_port/extern/PsyCross/src/psx/LIBGPU.C','pc_port/extern/PsyCross/src/psx/LIBGTE.C',
 'pc_port/extern/PsyCross/src/psx/INLINE_C.C','pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp',
 'pc_port/tests/battle_shard_callbacks_host_test.c','pc_port/tests/run_battle_shard_callbacks_host_test.sh']
source=pathlib.Path('src/slus_006.64/system/animation_scripts.c').read_text()
start=source.index('s32 func_80021AD8(s32 color, s32 value) {');end=source.index('\n}',start)+2
helper=source[start:end]
(out/'color.c').write_text('#include <stdint.h>\ntypedef int32_t s32;\n'+helper+'\n')
(out/'provenance.json').write_text(json.dumps({'scope':'Original callback callers vs native bodies; modeled and real native GTE SDK. Original matrix/tag leaf oracles. Not hardware graphics or playable battle acceptance.',
 'regions':regions,'color_body_sha256':hashlib.sha256(helper.encode()).hexdigest(),
 'source_pins':{p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in paths}},indent=2)+'\n')
PY
python3 tools/scripts/gen_battle_bridge_map.py --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"
common=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -include assert.h -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
sdk_common=(-std=c++17 -fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -fpermissive -w -include pc_port/src/port_compat.h -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"
for mode in UBSan O0 O2; do
    compiler=gcc; flags=(-"$mode")
    if [ "$mode" = UBSan ]; then compiler=clang; flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
    for spec in leaves:pc_port/src/retail_leaf_adapters.c test:pc_port/tests/battle_shard_callbacks_host_test.c \
                cpu:pc_port/src/battle_mips_adapter.c vblank:pc_port/src/controller_vblank_service.c color:"$OUT/color.c"; do
        "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "${spec#*:}" -o "$OUT/$mode.${spec%%:*}.o"
    done
    sdk_objects=()
    for unit in psx/LIBGPU.C psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${unit##*/}; sdk_objects+=("$OUT/$mode.$name.o")
        g++ "${sdk_common[@]}" "${sdl_flags[@]}" "${flags[@]}" -c "pc_port/extern/PsyCross/src/$unit" -o "$OUT/$mode.$name.o"
    done
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.leaves.o" \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" "$OUT/$mode.color.o" \
        "${sdk_objects[@]}" -ldl -lcrypto -o "$OUT/$mode.test"
    for suite in model sdk leaf; do
        args=(); if [ "$suite" != model ]; then args=(--"$suite"); fi
        if ! "$OUT/$mode.test" "${args[@]}" > "$OUT/$mode.$suite.log" 2>&1; then cat "$OUT/$mode.$suite.log" >&2; exit 1; fi
        tail -n 2 "$OUT/$mode.$suite.log"
    done
done
python3 - "$OUT" <<'PY'
import pathlib,sys
out=pathlib.Path(sys.argv[1]);runtime=pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body=pathlib.Path('pc_port/src/battle_shard_callbacks_host.inc').read_text()
mutations={
 'wrong-delay':('delay - 1u','delay - 2u'),
 'wrong-buffer':('(SC_R32(0x800ccb34u) << 5)','(SC_R32(0x800ccb34u) << 4)'),
 'unsigned-velocity':('(uint32_t)(int32_t)(int16_t)SC_R16(SC_S1 + 0x6au)','SC_R16(SC_S1 + 0x6au)'),
 'floor-damping':('(int32_t)yspeed / 16','(int32_t)yspeed >> 4'),
 'wrong-gravity':('SC_R32(SC_S1 + 0x78u)','SC_R32(SC_S1 + 0x74u)'),
 'early-depth-cull':('>= 64','> 64'),
 'wrong-vertices':('SC_S7 == 0 ?','SC_S7 != 0 ?'),
 'wrong-output-slot':('sp + 0x48u, sp + 0x4cu','sp + 0x4cu, sp + 0x48u'),
 'unsigned-depth':('((int32_t)depth >> 6)','(depth >> 6)'),
 'lost-geom-restore':('SC_R32(sp + 0x54u));','512);'),
 'wrong-free-pointer':('0x800b7348u, SC_S0','0x800b7348u, SC_S0 + 4u'),
 'unguarded-identity':('identity[i * 2 + 1]) return 0;','identity[i * 2 + 1]) break;')}
for name,(old,new) in mutations.items():
 assert old in body,name
 p=out/(name+'.inc');p.write_text(body.replace(old,new))
 (out/(name+'.c')).write_text(runtime.replace('#include "battle_shard_callbacks_host.inc"','#include "'+str(p)+'"'))
for name,old,new in [('wrong-q12','return (int32_t)((uint32_t)a * (uint32_t)b) >> 12;','return (int32_t)((uint32_t)a * (uint32_t)b) / 4096;'),
 ('host-packet-link','(cpu->gpr[5] & 0x00ffffffu)','((uint32_t)(uintptr_t)resolve_memory(runtime, b, 4, 0) & 0x00ffffffu)')]:
 assert runtime.count(old)==1,name
 (out/(name+'.c')).write_text(runtime.replace(old,new))
PY
for mutant in wrong-delay wrong-buffer unsigned-velocity floor-damping wrong-gravity early-depth-cull wrong-vertices \
              wrong-output-slot unsigned-depth lost-geom-restore wrong-free-pointer unguarded-identity wrong-q12 host-packet-link; do
    gcc "${common[@]}" -O2 -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
        -c pc_port/tests/battle_shard_callbacks_host_test.c -o "$OUT/$mutant.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" "$OUT/O2.leaves.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.vblank.o" "$OUT/O2.color.o" "$OUT/O2.LIBGPU.C.o" \
        "$OUT/O2.LIBGTE.C.o" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -lcrypto -o "$OUT/$mutant.test"
    args=(); if [ "$mutant" = wrong-q12 ] || [ "$mutant" = host-packet-link ]; then args=(--leaf); fi
    status=0; "$OUT/$mutant.test" "${args[@]}" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" != 1 ] || ! rg -q '^BATTLE SHARD CALLBACKS FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2; printf 'Unexpected control result %s status=%s\n' "$mutant" "$status" >&2; exit 1
    fi
    printf 'BATTLE SHARD CALLBACKS negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PY'
import hashlib,json,pathlib,sys
out=pathlib.Path(sys.argv[1])
for p,d in json.loads((out/'provenance.json').read_text())['source_pins'].items():
 assert hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()==d,p
print('BATTLE SHARD CALLBACKS source pins unchanged')
PY
