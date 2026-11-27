#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
image=${XENO_SLUS:-disc/SLUS_006.64}
digest=$(sha256sum < "$image")
[[ ${digest%% *} == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]]
build=$(mktemp -d "${TMPDIR:-/tmp}/xeno-popup-matrix.XXXXXXXX")
trap 'rm -rf "$build"' EXIT
ulimit -c 0
"${CC:-cc}" -std=c11 -D_POSIX_C_SOURCE=200809L -O2 -Wall -Wextra -Werror \
    -fno-pie -no-pie -Ipc_port/src pc_port/tests/battle_popup_transmatrix_test.c \
    pc_port/src/battle_mips_adapter.c -o "$build/test"
env -u XENO_DISABLE_CPU_INTERPRETER timeout 60 "$build/test" "$image"
