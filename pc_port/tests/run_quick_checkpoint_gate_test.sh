#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
for mode in O0 O2 UBSan; do
  flags=(-"$mode")
  if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
  "${CC:-clang}" -std=gnu17 -Wall -Wextra "${flags[@]}" -DXENO_PC_PORT \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -Iinclude -Ipc_port/include -Ipc_port/src \
    pc_port/tests/quick_checkpoint_gate_test.c \
    "${XENO_CHECKPOINT_TEST_SOURCE:-pc_port/src/quick_checkpoint.c}" \
    pc_port/src/quick_checkpoint_file.c -o "$out/test"
  "$out/test" "$out/checkpoint"
done
