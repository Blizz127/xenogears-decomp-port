#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/clip_pose_opcode_retail_test
mkdir -p "$OUT"
for opt in O0 O2 UBSan; do
  flags=(-"$opt")
  if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
  clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -fno-pie -no-pie "${flags[@]}" \
    pc_port/tests/clip_pose_opcode_retail_test.c pc_port/src/field_clip_data.c \
    -o "$OUT/$opt.test"
  "$OUT/$opt.test"
done
echo 'CLIP OPCODES 10/11/18 O0/O2/UBSAN PASS'
