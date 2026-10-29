#!/usr/bin/env bash
# Fixed fixture-bus comparisons; services are simulated and not gameplay proof.
set -euo pipefail
if [[ ${XENO_COMMAND_MENU_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_COMMAND_MENU_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-command-menu.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/battle.bin | cut -d' ' -f1) == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
python3 - "$OUT" <<'PY'
from pathlib import Path
import re,sys
out=Path(sys.argv[1]);s=Path('pc_port/src/battle_command_menu_host.inc').read_text()
marker='\n/* Lifted calls use the synthetic halt return, not a retail call-site address. */'
assert s.count(marker)==1
body=s.split(marker,1)[0];(out/'battle_command_menu_body.inc').write_text(body+'\n')
pairs={(int(a,16),int(b,16)+8) for a,b in re.findall(r'battle_phase_call\(entry,\s*(0x[0-9a-f]+)u,\s*(0x[0-9a-f]+)u\)',body)}
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text()
block=runtime.split('/* These experimental menu edges cannot bind unverified legacy C. */',1)[1].split('const char *menu_sound_experiment',1)[0]
menu_edges=block.split('))) {',1)[0]
guards={(int(a,16),int(b,16)) for a,b in re.findall(r'target==(0x[0-9a-f]+)u && cpu->gpr\[31\]==(0x[0-9a-f]+)u',menu_edges)}
assert len(pairs)==23 and pairs==guards and 'XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT' in block and 'return bridge_adopted_leaf_or(0,target);' in block
assert 'battle_attack_ring_try_host(runtime,cpu,target)' in block
assert 'battle_target_mask_try_host(runtime,cpu,target)' in block
assert 'battle_actor_clear_flag_try_host(runtime,cpu,target)' in block
assert 'battle_menu_reset_try_host(runtime,cpu,target)' in block
assert 'battle_inactive_free_try_host(runtime,cpu,target)' in block
assert 'battle_active_free_try_host(runtime,cpu,target)' in block
assert 'battle_menu_free_try_host(runtime,cpu,target)' in block
assert 'cpu->gpr[31] == 0x80080150u' in block
assert 'target == 0x800320e8u' in block
assert 'battle_inactive_state_try_host(runtime,cpu,target)' in block
assert 'battle_active_state_try_host(runtime,cpu,target)' in block
assert 'target == 0x8008fa60u && cpu->gpr[31] == 0x8007fbb8u' in block
assert 'battle_small_cursor_try_host(runtime,cpu,target)' in block
assert 'battle_cursor_layout_try_host(runtime,cpu,target)' in block
assert 'battle_empty_menu_try_host(runtime,cpu,target)' in block
assert 'battle_menu_alloc_try_host(runtime,cpu,target)' in block
assert 'target == 0x8008abb8u && cpu->gpr[31] == 0x8007fed8u' in block
assert 'XENO_BATTLE_NATIVE_MENU_CAMERA_EXPERIMENT' in block and 'battle_target_camera_try_host(runtime,cpu,target)' in block
PY
for kind in four_state two_frame fault default early_exit; do
    env -u XENO_DISABLE_CPU_INTERPRETER gcc -std=gnu17 -O0 -fno-pie -no-pie \
        -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -include assert.h -ffunction-sections -fdata-sections \
        -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT" \
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
        -w "pc_port/tests/battle_command_menu_${kind}_test.c" pc_port/src/battle_mips_adapter.c \
        -Wl,--gc-sections -o "$OUT/$kind"
    env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$kind" disc/battle.bin
done
gcc -std=gnu17 -O0 -fno-pie -no-pie -Ipc_port/src \
    pc_port/tests/battle_command_menu_gate_test.c -lcrypto -o "$OUT/gate"
env -u XENO_BATTLE_NATIVE_COMMAND_MENU_EXPERIMENT "$OUT/gate" disc/battle.bin
