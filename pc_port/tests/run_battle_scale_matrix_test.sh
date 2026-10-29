#!/usr/bin/env bash
# Fixed sign, alias, and bus-fault cases on a synthetic guest bus.
set -euo pipefail
if [[ ${XENO_SCALE_MATRIX_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_SCALE_MATRIX_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-scale-matrix.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/SLUS_006.64 | cut -d' ' -f1) == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]]
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
s=Path('pc_port/src/battle_scale_matrix_host.inc').read_text();marker='\n/* Restricted to the atlas ScaleMatrixL edge with aligned accessible spans. */'
assert s.count(marker)==1
Path(sys.argv[1],'battle_scale_matrix_body.inc').write_text(s.split(marker,1)[0]+'\n')
runtime=Path('pc_port/src/battle_mips_runtime.c').read_text();block=runtime.split('/* Atlas SDK calls refuse unless separately converted and verified. */',1)[1].split('/* A decline cannot bind legacy C at these packet edges. */',1)[0]
assert 'target==0x8004974cu && cpu->gpr[31]==0x80026074u' in block and 'battle_scale_matrix_try_host(runtime,cpu,target)' in block and 'return -1;' in block
PY
env -u XENO_DISABLE_CPU_INTERPRETER gcc -std=gnu17 -O0 -fno-pie -no-pie \
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
    -include assert.h -ffunction-sections -fdata-sections \
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT" \
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
    -w pc_port/tests/battle_scale_matrix_test.c pc_port/src/battle_mips_adapter.c \
    -Wl,--gc-sections -o "$OUT/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/test" disc/SLUS_006.64
gcc -std=gnu17 -O0 -fno-pie -no-pie -Ipc_port/src pc_port/tests/battle_scale_matrix_gate_test.c -lcrypto -o "$OUT/gate"
env -u XENO_BATTLE_NATIVE_SCALE_MATRIX_EXPERIMENT "$OUT/gate" disc/SLUS_006.64
