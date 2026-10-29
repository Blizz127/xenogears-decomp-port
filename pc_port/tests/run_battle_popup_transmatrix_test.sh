#!/usr/bin/env bash
set -euo pipefail
if [[ ${XENO_TRANS_CALLBACK_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_TRANS_CALLBACK_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
out=$(mktemp -d "${TMPDIR:-/tmp}/xeno-trans-callback.XXXXXX")
trap 'rm -rf "$out"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/SLUS_006.64 | cut -d' ' -f1) == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]]
python3 - <<'PYCODE'
from pathlib import Path
s=Path('pc_port/src/battle_mips_runtime.c').read_text()
block=s.split('const char *popup_transmatrix=',1)[1].split('/* Atlas SDK calls',1)[0]
assert 'target==0x80049d9cu' in block and 'cpu->gpr[31]==0x800bceccu' in block
assert 'popup_transmatrix_try_host(runtime,cpu,target)' in block and 'return hosted!=0?hosted:-1;' in block
assert '#include "battle_popup_transmatrix_host.inc"' in s
PYCODE
env -u XENO_DISABLE_CPU_INTERPRETER gcc -std=gnu17 -O0 -fno-pie -no-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx pc_port/tests/battle_popup_transmatrix_test.c pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -lcrypto -o "$out/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$out/test"
