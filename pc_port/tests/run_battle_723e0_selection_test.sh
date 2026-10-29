#!/usr/bin/env bash
# Four synthetic actor-selection comparisons against retail instructions.
set -euo pipefail
if [[ ${XENO_723E0_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 120s env XENO_723E0_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-723e0.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
ulimit -c 0
ulimit -v 12582912
[[ $(sha256sum disc/battle.bin | cut -d' ' -f1) == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
env -u XENO_DISABLE_CPU_INTERPRETER gcc -std=gnu17 -O0 -fno-pie -no-pie \
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 \
    -include assert.h -ffunction-sections -fdata-sections \
    -Ipc_port/include_shim -Iinclude -Ipc_port/src \
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
    -w pc_port/tests/battle_723e0_selection_test.c src/battle/main28f0_port.c \
    pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -o "$OUT/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/test" disc/battle.bin
