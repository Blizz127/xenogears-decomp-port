#!/usr/bin/env bash
# Fixed synthetic geometry and bus-fault comparisons; no rendering proof.
set -euo pipefail
if [[ ${XENO_GEOMETRY_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_GEOMETRY_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-frame-geometry.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/battle.bin | cut -d' ' -f1) == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
s=Path('pc_port/src/battle_frame_geometry_host.inc').read_text();marker='\n/* Restricted to the observed HUD frame call. */'
assert s.count(marker)==1
Path(sys.argv[1],'battle_frame_geometry_body.inc').write_text(s.split(marker,1)[0]+'\n')
PY
env -u XENO_DISABLE_CPU_INTERPRETER gcc -std=gnu17 -O0 -fno-pie -no-pie \
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
    -include assert.h -ffunction-sections -fdata-sections \
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT" \
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
    -w pc_port/tests/battle_frame_geometry_test.c pc_port/src/battle_mips_adapter.c \
    -Wl,--gc-sections -o "$OUT/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/test" disc/battle.bin
gcc -std=gnu17 -O0 -fno-pie -no-pie -Ipc_port/src pc_port/tests/battle_frame_geometry_gate_test.c -lcrypto -o "$OUT/gate"
env -u XENO_BATTLE_NATIVE_FRAME_GEOMETRY_EXPERIMENT "$OUT/gate" disc/battle.bin
