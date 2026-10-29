#!/usr/bin/env bash
# Synthetic HUD, panel and window link comparisons on a fixture bus.
set -euo pipefail
if [[ ${XENO_STRIP_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_STRIP_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-strip.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/battle.bin | cut -d' ' -f1) == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
[[ $(sha256sum disc/SLUS_006.64 | cut -d' ' -f1) == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]]
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
out=Path(sys.argv[1]);s=Path('pc_port/src/battle_optional_strips_host.inc').read_text()
a=s.index('static void battle_optional_strips_link(');b=s.index('\nstatic void battle_optional_strips_add_prim(',a)
(out/'link.c').write_text(s[a:b]+'\n')
s=Path('pc_port/src/battle_fade_draw_host.inc').read_text()
a=s.index('#define FD_R(');b=s.index('\nstatic void battle_fade_draw_call',a);macros=s[a:b]
a=s.index('static void battle_fade_draw_sdk_run(');b=s.index('\nstatic int battle_fade_draw_sdk_try_host',a)
(out/'addprim-body.c').write_text(macros+'\nstatic void battle_fade_draw_mode_word(BattleEntryCall *e){(void)e;abort();}\nstatic void battle_fade_draw_window_word(BattleEntryCall *e){(void)e;abort();}\n'+s[a:b]+'\n')
PY
env -u XENO_DISABLE_CPU_INTERPRETER gcc -std=gnu17 -O0 -fno-pie -no-pie \
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
    -include assert.h -ffunction-sections -fdata-sections \
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT" \
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
    -w pc_port/tests/battle_728b8_strip_test.c pc_port/src/battle_mips_adapter.c \
    -Wl,--gc-sections -o "$OUT/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/test" disc/battle.bin disc/SLUS_006.64
env -u XENO_DISABLE_CPU_INTERPRETER gcc -std=gnu17 -O0 -fno-pie -no-pie \
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
    -include assert.h -ffunction-sections -fdata-sections \
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT" \
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
    -w pc_port/tests/battle_addprim_sites_test.c pc_port/src/battle_mips_adapter.c \
    -Wl,--gc-sections -o "$OUT/addprim-test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/addprim-test" disc/SLUS_006.64
