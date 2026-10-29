#!/usr/bin/env bash
# Synthetic sound continuation comparison; real sound events are never called.
set -euo pipefail
if [[ ${XENO_SOUND_CONTINUATION_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 90s env XENO_SOUND_CONTINUATION_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-sound-continuation.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/SLUS_006.64 | cut -d' ' -f1) == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]]
python3 - "$OUT" <<'PY'
from pathlib import Path
import re,sys
out=Path(sys.argv[1])
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text()
marker='static int runtime_read(void *opaque, uint32_t address, unsigned width,\n'
assert runtime.count(marker)==1
runtime=runtime.replace(marker, '#include "slot-projection.inc"\n\n'+marker, 1)
marker='    BattleMipsRuntime *runtime = opaque;\n    uint8_t *p = address < BATTLE_BUS_KUSEG_RAM_END'
assert runtime.count(marker)==2
runtime=runtime.replace(marker,'    BattleMipsRuntime *runtime = opaque;\n    const char *projection=getenv("XENO_SCRATCH_SOUND_POINTER_PROJECTION");if(projection&&!strcmp(projection,"1")){int projected=scratch_sound_project_read(runtime,address,width,value);if(projected)return projected<0?-1:0;}\n    uint8_t *p = address < BATTLE_BUS_KUSEG_RAM_END',1)
marker='    BattleMipsRuntime *runtime = opaque;\n    uint8_t *p = address < BATTLE_BUS_KUSEG_RAM_END'
runtime=runtime.replace(marker,'    BattleMipsRuntime *runtime = opaque;\n    const char *projection=getenv("XENO_SCRATCH_SOUND_POINTER_PROJECTION");if(projection&&!strcmp(projection,"1")){int projected=scratch_sound_project_write(runtime,address,width,value);if(projected)return projected<0?-1:0;}\n    uint8_t *p = address < BATTLE_BUS_KUSEG_RAM_END',1)
(out/'battle-runtime-projection.c').write_text(runtime)

PY
COMMON=(-fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/build_native_only -I"$OUT" -Ipc_port/tests/native_sound_continuation)
for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
    g++ -std=c++17 "${COMMON[@]}" -O0 -fpermissive -w -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$OUT/${source##*/}.o"
done
for pair in "cpu:pc_port/src/battle_mips_adapter.c"; do
    name=${pair%%:*}; source=${pair#*:}; extra=(-include common.h -include psyq/libgpu.h -include psyq/libgte.h -include psx_memory.h)
    if [[ $name == cpu ]]; then extra=(); fi
    gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w "${extra[@]}" -c "$source" -o "$OUT/$name.o"
done
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -fpermissive -w -c pc_port/src/psyq_compat.c -o "$OUT/compat.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w -c pc_port/src/retail_leaf_adapters.c -o "$OUT/leaf.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w -include common.h -include psyq/libgpu.h -include psyq/libgte.h -include psx_memory.h -c pc_port/tests/native_sound_continuation/projected-continuation-test.c -o "$OUT/test.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w -c pc_port/src/controller_vblank_service.c -o "$OUT/vblank.o"
clang++ -Wl,--export-dynamic-symbol=D_80059478 -no-pie -O0 -Wl,--gc-sections "$OUT/test.o" "$OUT/vblank.o" "$OUT/cpu.o" "$OUT/compat.o" "$OUT/leaf.o" "$OUT/LIBGTE.C.o" "$OUT/INLINE_C.C.o" "$OUT/PsyX_GTE.cpp.o" -ldl -o "$OUT/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/test"
