#!/usr/bin/env bash
# Fixed wrapper fixtures; real audio service remains guarded.
set -euo pipefail
if [[ ${XENO_MENU_SOUND_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_MENU_SOUND_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-menu-sound.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/battle.bin | cut -d' ' -f1) == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
s=Path('pc_port/src/battle_menu_sound_host.inc').read_text();marker='\nstatic int battle_menu_sound_try_host('
assert s.count(marker)==1
Path(sys.argv[1],'battle_menu_sound_body.inc').write_text(s.split(marker,1)[0]+'\n')
enabled=Path('pc_port/src/battle_sound_enabled_host.inc').read_text();mark='\n/* Enabled wrapper only; actual sound service remains independently refused. */'
assert enabled.count(mark)==1
Path(sys.argv[1],'battle_sound_enabled_body.inc').write_text(enabled.split(mark,1)[0]+'\n')
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text()
assert 'target==0x8003b644u && cpu->gpr[31]==0x80039e08u' in runtime
PY
for name in battle_menu_sound_test battle_menu_sound_fault_test battle_sound_enabled_test; do
    env -u XENO_DISABLE_CPU_INTERPRETER gcc -std=gnu17 -O0 -fno-pie -no-pie \
        -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -include assert.h -ffunction-sections -fdata-sections \
        -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT" \
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
        -w "pc_port/tests/$name.c" pc_port/src/battle_mips_adapter.c \
        -Wl,--gc-sections -o "$OUT/$name"
    if [[ $name == battle_sound_enabled_test ]]; then
        env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$name"
    else
        env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$name" disc/battle.bin
    fi
done
gcc -std=gnu17 -O0 -fno-pie -no-pie -Ipc_port/src pc_port/tests/battle_menu_sound_gate_test.c -lcrypto -o "$OUT/gate"
env -u XENO_BATTLE_NATIVE_MENU_SOUND_EXPERIMENT "$OUT/gate" disc/battle.bin
