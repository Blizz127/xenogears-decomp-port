#!/usr/bin/env bash
# Fixed sprite fixtures with simulated rendering services; no visual proof.
set -euo pipefail
if [[ ${XENO_FRAME_SPRITE_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_FRAME_SPRITE_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-frame-sprite.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/battle.bin | cut -d' ' -f1) == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
s=Path('pc_port/src/battle_frame_sprite_host.inc').read_text();marker='\n/* Restricted to the observed HUD frame caller. */'
assert s.count(marker)==1
Path(sys.argv[1],'battle_frame_sprite_body.inc').write_text(s.split(marker,1)[0]+'\n')
import re
body=s.split(marker,1)[0]
pairs={(int(a,16),int(b,16)+8) for a,b in re.findall(r'battle_phase_call\(entry,\s*(0x[0-9a-f]+)u,\s*(0x[0-9a-f]+)u\)',body)}
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text()
block=runtime.split('const char *sprite_experiment =',1)[1].split('/* Atlas SDK calls refuse unless separately converted and verified. */',1)[0]
assert pairs=={(0x80025fa8,0x80076814),(0x80025fa8,0x8007688c),(0x80043bfc,0x800768f0)}
assert all(f'0x{v:08x}u' in block for pair in pairs for v in pair)
assert 'XENO_BATTLE_NATIVE_FRAME_SPRITE_EXPERIMENT' in block and 'return -1;' in block
PY
env -u XENO_DISABLE_CPU_INTERPRETER gcc -std=gnu17 -O0 -fno-pie -no-pie \
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
    -include assert.h -ffunction-sections -fdata-sections \
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT" \
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
    -w pc_port/tests/battle_frame_sprite_test.c pc_port/src/battle_mips_adapter.c \
    -Wl,--gc-sections -o "$OUT/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/test" disc/battle.bin
gcc -std=gnu17 -O0 -fno-pie -no-pie -Ipc_port/src pc_port/tests/battle_frame_sprite_gate_test.c -lcrypto -o "$OUT/gate"
env -u XENO_BATTLE_NATIVE_FRAME_SPRITE_EXPERIMENT "$OUT/gate" disc/battle.bin
