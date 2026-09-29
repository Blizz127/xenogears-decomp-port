#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/clip_control_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from tools.scripts.audit_field_clip_vm import inventory,read_overlay
inventory(read_overlay('disc/disc1.bin')) # pins full VM, overlay and dispatch
PY
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -fno-pie -no-pie "${flags[@]}" pc_port/tests/clip_control_retail_test.c pc_port/src/field_clip_control.c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
ulimit -c 0
for mutant in limit signed rewind postprocess unhandled noop status_bit event_bit event_signed; do
 case "$mutant" in
  limit) expression='s/state->limit != -1/state->limit == -1/' ;;
  signed) expression='s/(int16_t)\*elapsed >= (int16_t)duration/(uint16_t)*elapsed >= (uint16_t)duration/' ;;
  rewind) expression='s/state->stream = start;/state->stream = start + 2u;/' ;;
  postprocess) expression='s/state->postprocess = 1;/state->postprocess = 0;/' ;;
  unhandled) expression='s/if (opcode < 113) return 0/if (opcode < 113) return 1/' ;;
  noop) expression='s/case 6: case 7:/case 8: case 7:/' ;;
  status_bit) expression='s/opcode == 0x20 ? 0x100 : 1/opcode == 0x20 ? 0x200 : 2/' ;;
  event_bit) expression='s/parameter == 255 ? 0x400 : 4/parameter == 255 ? 0x200 : 2/' ;;
  event_signed) expression='s/(int32_t)\*count >= (int16_t)duration/(int16_t)*count >= (int16_t)duration/' ;;
 esac
 sed "$expression" pc_port/src/field_clip_control.c > "$OUT/$mutant.c"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -fno-pie -no-pie -O2 pc_port/tests/clip_control_retail_test.c "$OUT/$mutant.c" pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "CLIP CONTROL mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'CLIP CONTROL FAIL' "$OUT/$mutant.log"
done
echo 'CLIP CONTROL negative controls PASS: limit signed rewind postprocess unhandled noop status_bit event_bit event_signed'
