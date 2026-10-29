#!/usr/bin/env bash
# Fixed camera-wrapper comparisons; its child services are simulated.
set -euo pipefail
if [[ ${XENO_MENU_CAMERA_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_MENU_CAMERA_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
ulimit -c 0
ulimit -v 12582912
python3 - <<'PY'
import hashlib,os,subprocess,tempfile
from pathlib import Path
s=Path('pc_port/src/battle_target_camera_host.inc').read_text();a=s.index('#define TC_R32');b=s.index('static uint32_t battle_target_camera_asr',a);macros=s[a:b];a=s.index('static void battle_target_camera_wrapper');b=s.index('static void battle_target_camera_view',a);body=s[a:b]
with Path('disc/battle.bin').open('rb') as f:f.seek(0x4c914);code=f.read(80)
assert hashlib.sha256(code).hexdigest()=='bf56810e8fd31bea70329998fcaf4757aa7c7700b88b773c1d1aee47ce91e8d8'
with tempfile.TemporaryDirectory(prefix='xeno-menu-camera-') as out:
 Path(out,'body.inc').write_text(macros+body)
 cmd=['gcc','-std=gnu17','-O0','-I'+out,'-Ipc_port/src','-Ipc_port/include_shim','-Iinclude','-Ipc_port/extern/PsyCross/include','-Ipc_port/extern/PsyCross/include/psx','pc_port/tests/battle_menu_camera_test.c','pc_port/src/battle_mips_adapter.c','-o',out+'/test']
 subprocess.run(cmd,check=True)
 env=os.environ.copy();env.pop('XENO_DISABLE_CPU_INTERPRETER',None)
 subprocess.run([out+'/test'],check=True,env=env)
PY
