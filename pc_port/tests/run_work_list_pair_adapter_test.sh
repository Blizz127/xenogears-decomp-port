#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT="$(mktemp -d -t work-list-pair.XXXXXX)"
trap 'rm -rf -- "$OUT"' EXIT

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0 -fno-pie -no-pie
      -ffunction-sections -fdata-sections -Wall -Wextra)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src
     -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

for opt in O0 O2; do
    gcc "${BASE[@]}" "-${opt}" "${INC[@]}" \
        pc_port/src/work_list_port.c \
        pc_port/tests/work_list_pair_adapter_test.c \
        -Wl,--gc-sections -o "$OUT/test-${opt}"
    "$OUT/test-${opt}" >"$OUT/${opt}.log" 2>&1
done
cmp "$OUT/O0.log" "$OUT/O2.log"
cat "$OUT/O0.log"
