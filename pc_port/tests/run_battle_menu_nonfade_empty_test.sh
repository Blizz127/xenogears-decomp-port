#!/usr/bin/env bash
# Fixed synthetic atlas/sprite comparisons; GPU CPU services are test-only.
set -euo pipefail
if [[ ${XENO_ATLAS_GUESTABI_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 90s env XENO_ATLAS_GUESTABI_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-atlas-abi.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/SLUS_006.64 | cut -d' ' -f1) == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]]
[[ $(sha256sum disc/battle.bin | cut -d' ' -f1) == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
python3 - "$OUT" <<'PY'
from pathlib import Path
import re,sys
out=Path(sys.argv[1]);s=Path('pc_port/src/battle_atlas_guestabi_host.inc').read_text();marker='\n/* Restricted to the two frame-sprite atlas calls and the verified SLUS image. */'
assert s.count(marker)==1
body=s.split(marker,1)[0];(out/'battle_atlas_guestabi_body.inc').write_text(body+'\n')
pairs={(int(a,16),int(b,16)+8) for a,b in re.findall(r'battle_phase_call\(entry,\s*(0x[0-9a-f]+)u,\s*(0x[0-9a-f]+)u\)',body)}
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text();block=runtime.split('/* Atlas SDK calls refuse unless separately converted and verified. */',1)[1].split('/* A decline cannot bind legacy C at these packet edges. */',1)[0]
guards={(int(a,16),int(b,16)) for a,b in re.findall(r'target==(0x[0-9a-f]+)u && cpu->gpr\[31\]==(0x[0-9a-f]+)u',block)}
assert len(pairs)==18 and pairs==guards and 'XENO_BATTLE_NATIVE_ATLAS_GUESTABI_EXPERIMENT' in block and 'return -1;' in block
push=Path('pc_port/src/battle_pushmatrix_host.inc').read_text();mark='\nextern int matrixLevel;'
assert push.count(mark)==1
(out/'battle_pushmatrix_body.inc').write_text(push.split(mark,1)[0]+'\n')
assert 'battle_pushmatrix_try_host(runtime,cpu,target)' in block
geom=Path('pc_port/src/battle_gte_geometry_host.inc').read_text();mark='\n/* Only verified atlas geometry-control call sites. */'
assert geom.count(mark)==1
(out/'battle_gte_geometry_body.inc').write_text(geom.split(mark,1)[0]+'\n')
assert 'battle_geom_control_try_host(runtime,cpu,target)' in block
matrix=Path('pc_port/src/battle_gte_matrix_host.inc').read_text();mark='\n/* Only the two atlas matrix-setter call sites. */'
assert matrix.count(mark)==1
(out/'battle_gte_matrix_body.inc').write_text(matrix.split(mark,1)[0]+'\n')
assert 'battle_set_matrix_try_host(runtime,cpu,target)' in block
assert 'battle_atlas_gpu_try_host(runtime,cpu,target)' in block
PY
sed -n '/^s32 func_80025FA8(/,/^}/p' src/slus_006.64/system/temp1f.c > "$OUT/body.c"
sed -n -e '/^u_short GetClut(/,/^}/p' -e '/^u_short GetTPage(/,/^}/p' \
    -e '/^void SetSemiTrans(/,/^}/p' -e '/^void SetPolyFT4(/,/^}/p' \
    -e '/^void SetShadeTex(/,/^}/p' pc_port/extern/PsyCross/src/psx/LIBGPU.C > "$OUT/gpu.c"
COMMON=(-fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/build_native_only -I"$OUT")
for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
    g++ -std=c++17 "${COMMON[@]}" -O0 -fpermissive -w -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/${source##*/}.o"
done
for pair in "cpu:pc_port/src/battle_mips_adapter.c" "body:$OUT/body.c" "gpu:$OUT/gpu.c"; do
    name=${pair%%:*}; source=${pair#*:}; extra=(-include common.h -include psyq/libgpu.h -include psyq/libgte.h -include psx_memory.h)
    if [[ $name == cpu ]]; then extra=(); fi
    gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w "${extra[@]}" -c "$source" -o "$OUT/$name.o"
done
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -fpermissive -w -c pc_port/src/psyq_compat.c -o "$OUT/compat.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w -c pc_port/src/retail_leaf_adapters.c -o "$OUT/leaf.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w -include common.h -include psyq/libgpu.h -include psyq/libgte.h -include psx_memory.h -c pc_port/tests/battle_menu_nonfade_empty_gate_test.c -o "$OUT/test.o"
clang++ -no-pie -O0 -Wl,--gc-sections "$OUT/test.o" "$OUT/cpu.o" "$OUT/body.o" "$OUT/gpu.o" "$OUT/compat.o" "$OUT/leaf.o" "$OUT/LIBGTE.C.o" "$OUT/INLINE_C.C.o" "$OUT/PsyX_GTE.cpp.o" -ldl -o "$OUT/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/test"
