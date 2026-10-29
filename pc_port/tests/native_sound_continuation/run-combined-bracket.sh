#!/usr/bin/env bash
# Synthetic continuation compared with a real SDL sound-event bracket.
set -euo pipefail
if [[ ${XENO_COMBINED_BRACKET_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 90s env XENO_COMBINED_BRACKET_BOUNDED=1 "$0" "$@"
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
mkdir -p "$OUT/src/psx"
cp pc_port/extern/PsyCross/src/psx/LIBAPI.C "$OUT/src/psx/LIBAPI.C"
for patch_name in reservation reservation_identity reservation_cleanup reservation_epoch reservation_owned reservation_pristine; do
    if ! rg -q "_xeno_sound_${patch_name}" "$OUT/src/psx/LIBAPI.C"; then patch -s -d "$OUT" -p1 < "pc_port/patches/psycross_sound_${patch_name}.patch"; fi
done
read -r -a SDL_FLAGS <<< "$(pkg-config --cflags sdl2)"
read -r -a SDL_LIBS <<< "$(pkg-config --libs sdl2)"
g++ -std=c++17 -O1 -fpermissive -w -ffunction-sections -fdata-sections -DPsyX_Sys_GetVBlankCount=CombinedVendorVBlankCount "${SDL_FLAGS[@]}" -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/extern/PsyCross/src/psx -c "$OUT/src/psx/LIBAPI.C" -o "$OUT/vendor.o"
COMMON=("${SDL_FLAGS[@]}" -fno-pie -fno-builtin -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
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
objcopy --weaken-symbol=PcPort_PrepareControllerPoll --weaken-symbol=PcPort_BeforeControllerPush "$OUT/compat.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -fpermissive -w -c src/slus_006.64/system/controller2.c -o "$OUT/controller.o"
objcopy --weaken-symbol=func_80035E44 --weaken-symbol=func_80036220 "$OUT/controller.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -fpermissive -w -c src/slus_006.64/system/sound.c -o "$OUT/sound.o"
objcopy --weaken-symbol=SoundHandleError --weaken-symbol=SoundReleaseAllVoices "$OUT/sound.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w -c pc_port/src/boot_sound_commit.c -o "$OUT/boot-sound.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w -c pc_port/src/retail_leaf_adapters.c -o "$OUT/leaf.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w -include common.h -include psyq/libgpu.h -include psyq/libgte.h -include psx_memory.h -c pc_port/tests/native_sound_continuation/combined-bracket-test.c -o "$OUT/test.o"
gcc "${COMMON[@]}" "${GAME[@]}" -O0 -w -c pc_port/src/controller_vblank_service.c -o "$OUT/vblank.o"
clang++ -Wl,--export-dynamic-symbol=D_80059478 -Wl,--wrap=SoundRemoveAudioManagerFromList -no-pie -O0 -Wl,--gc-sections "$OUT/test.o" "$OUT/vendor.o" "$OUT/vblank.o" "$OUT/controller.o" "$OUT/sound.o" "$OUT/boot-sound.o" "$OUT/cpu.o" "$OUT/compat.o" "$OUT/leaf.o" "$OUT/LIBGTE.C.o" "$OUT/INLINE_C.C.o" "$OUT/PsyX_GTE.cpp.o" -ldl "${SDL_LIBS[@]}" -o "$OUT/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/test"
