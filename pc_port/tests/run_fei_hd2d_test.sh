#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-cc}" -std=gnu17 -g -fno-pie -no-pie "${flags[@]}" \
        -DXENO_PC_PORT -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include \
        -Ipc_port/extern/PsyCross/include/psx pc_port/tests/fei_hd2d_test.c -o "$out/test"
    "$out/test"
done
