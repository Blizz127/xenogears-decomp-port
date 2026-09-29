#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/clip_data_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from tools.scripts.audit_field_clip_vm import inventory,read_overlay
inventory(read_overlay('disc/disc1.bin')) # pins full VM, overlay and dispatch
PY
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -fno-pie -no-pie "${flags[@]}" pc_port/tests/clip_data_retail_test.c pc_port/src/field_clip_data.c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in reset relative self_modify operand signed_vector q12 timer flag unhandled loop_signed loop_threshold motion_add position_width node_stride; do
 case "$mutant" in
  reset) expression='s/offset <= 0x86/offset < 0x86/' ;;
  relative) expression='s/parameter ? relative(start, state->operand) : 0/parameter ? start + (uint32_t)state->operand : 0/g' ;;
  self_modify) expression='s/put_half((void\*)(uintptr_t)state->stream, 0)/put_half((void*)(uintptr_t)state->stream, 1)/' ;;
  operand) expression='s/state->operand = instruction;/state->operand = 0;/' ;;
  signed_vector) expression='s/int32_t value = (int16_t)take(state);/int32_t value = (uint16_t)take(state);/' ;;
  q12) expression='s/>> 12/>> 11/g' ;;
  timer) expression='s/put_word(object + 0xa0, state->stream)/put_word(object + 0xa4, state->stream)/' ;;
  flag) expression='s/parameter \& 1/parameter \& 2/g' ;;
  unhandled) expression='s/return 0; \/\* Not/return 1; \/* Not/' ;;
  loop_signed) expression='s/(int16_t)count < (int32_t)(header >> 8)/(uint16_t)count < (int32_t)(header >> 8)/' ;;
  loop_threshold) expression='s/count < (int32_t)(header >> 8)/count <= (int32_t)(header >> 8)/' ;;
  motion_add) expression='s/previous + value/previous - value/' ;;
  position_width) expression='s/target != word(root + 0x5c + axis \* 4)/(uint16_t)target != (uint16_t)word(root + 0x5c + axis * 4)/' ;;
  node_stride) expression='s/\* 124u/* 120u/' ;;
 esac
 sed "$expression" pc_port/src/field_clip_data.c > "$OUT/$mutant.c"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -fno-pie -no-pie -O2 pc_port/tests/clip_data_retail_test.c "$OUT/$mutant.c" pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "CLIP DATA mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'CLIP DATA FAIL' "$OUT/$mutant.log"
done
echo 'CLIP DATA negative controls PASS: reset relative self_modify operand signed_vector q12 timer flag unhandled loop_signed loop_threshold motion_add position_width node_stride'
