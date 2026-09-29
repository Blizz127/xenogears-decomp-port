#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/timed_command_stack_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from hashlib import sha256
from tools.scripts.audit_field_clip_vm import read_overlay, OVERLAY_SHA
b=read_overlay('disc/disc1.bin')
assert sha256(b).hexdigest()==OVERLAY_SHA
assert sha256(b[0x9d44:0xa32c]).hexdigest()=='760f9f457085e59b63a6ae781297309641b8a4e429cc7ced348c1cda0f6c4c19'
PY
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 clang -std=c17 -Wall -Wextra -Werror "${flags[@]}" -Ipc_port/src pc_port/tests/timed_command_stack_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in zero-stack invert-branch; do
 if "$OUT/O2.test" "$mutant" > "$OUT/$mutant.log" 2>&1; then
  echo "TIMED STACK mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'TIMED STACK FAIL' "$OUT/$mutant.log"
done
echo 'TIMED STACK negative controls PASS: zero-stack invert-branch (RAM-only mutations)'
