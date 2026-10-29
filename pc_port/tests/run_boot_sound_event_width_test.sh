#!/usr/bin/env bash
set -euo pipefail
if [[ ${XENO_BOOT_EVENT_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_BOOT_EVENT_BOUNDED=1 bash "$0" "$@"
fi
cd "$(dirname "$0")/../.."
ulimit -c 0
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cc -std=gnu17 -include assert.h -Wall -Wextra -Werror -m64 -fno-builtin \
   -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -Iinclude -Ipc_port/src -O0 \
   pc_port/tests/boot_sound_commit_test.c pc_port/src/boot_sound_commit.c -o "$out/test"
timeout -k 5s 20s "$out/test"
