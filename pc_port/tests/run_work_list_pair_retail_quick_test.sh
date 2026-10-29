#!/usr/bin/env bash
# Focused paired-task comparison; deterministic heap, no production saves.
set -euo pipefail
if [[ ${XENO_PAIR_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 90s env XENO_PAIR_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../.."
ulimit -c 0
ulimit -v 12582912
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-pair-quick.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
read -r SHA _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x8001ca58-0x8000f800)) count=$((0x840)) status=none | sha256sum)
[[ $SHA == 29a6d9352b7866a46c03d45a76c182a1d813824e27ee9a2ef3525e7fb379ecfe ]]
COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
gcc "${COMMON[@]}" -O0 -w -c pc_port/tests/work_list_pair_retail_quick_test.c -o "$OUT/test.o"
gcc "${COMMON[@]}" -O0 -w -c pc_port/src/work_list_port.c -o "$OUT/native.o"
gcc "${COMMON[@]}" -O0 -w -c pc_port/src/battle_mips_adapter.c -o "$OUT/cpu.o"
gcc -no-pie -Wl,--gc-sections "$OUT/test.o" "$OUT/native.o" "$OUT/cpu.o" -o "$OUT/test"
"$OUT/test"
