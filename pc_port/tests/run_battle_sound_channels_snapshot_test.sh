#!/usr/bin/env bash
# Synthetic reservation and pointer checks for the retained channel diagnostic.
set -euo pipefail
cd "$(dirname "$0")/../.."

out=$(mktemp -d "${TMPDIR:-/tmp}/xeno-sound-snapshot.XXXXXXXX")
trap 'rm -rf "$out"' EXIT
renderer=pc_port/extern/PsyCross
python3 tools/scripts/gen_battle_bridge_map.py \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --out "$out/battle_bridge_map.inc"

# Compile this one test translation unit as a complete program. This keeps only
# the real resolver and diagnostic the fixture uses; no renderer or CPU bodies run.
gcc -fwhole-program -std=gnu17 -O2 -m64 -fno-pie -no-pie -fno-builtin \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -Wl,--export-dynamic-symbol=g_SoundChannels \
    -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C \
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h \
    -Ipc_port/src -Ipc_port/include_shim -Ipc_port/include -Iinclude -I"$out" \
    -I"$renderer/include" -I"$renderer/include/psx" -I"$renderer/src" \
    pc_port/tests/battle_sound_channels_snapshot_test.c -ldl -o "$out/test"
"$out/test"
