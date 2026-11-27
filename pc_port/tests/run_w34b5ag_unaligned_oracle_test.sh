#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
work=$(mktemp -d "${TMPDIR:-/tmp}/xeno-unaligned.XXXXXXXX")
trap 'rm -rf "$work"' EXIT
"${CC:-cc}" -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -O2 \
    -Ipc_port/src pc_port/tests/w34b5ag_unaligned_oracle_test.c \
    pc_port/src/battle_mips_adapter.c -o "$work/test"
env -u XENO_DISABLE_CPU_INTERPRETER "$work/test"
