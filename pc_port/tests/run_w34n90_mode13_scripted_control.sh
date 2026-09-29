#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n90_mode13_scripted_control"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n90_mode13_scripted_control_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_8032c.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x8008032C - 0x8006FAF0)) \
  count=$((0x80080578 - 0x8008032C)) status=none | \
  sha256sum | awk '{print $1}')"
expected="382507d7e94b9c25a128a1fadfefefadcd15ff2e2c8a88729fb272942a1157c7"
if [[ "$actual" != "$expected" ]]; then
  printf 'ERROR: retail slice SHA mismatch: %s\n' "$actual" >&2
  exit 1
fi

run_regime() {
  local name="$1"
  shift
  "$CC_BIN" "${COMMON[@]}" "$@" -o "$OUT/$name"
  "$OUT/$name"
}

run_mutant() {
  local name="$1"
  local define="$2"
  local assertion="$3"
  "$CC_BIN" "${COMMON[@]}" -O2 "$define" -o "$OUT/$name"
  if "$OUT/$name" >"$OUT/$name.log" 2>&1; then
    echo "ERROR: $name survived" >&2
    exit 1
  fi
  if ! grep -q "ASSERTION $assertion FAILED" "$OUT/$name.log"; then
    echo "ERROR: $name did not fail named assertion $assertion" >&2
    sed -n '1,220p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N90_MUTANT_WRONG_INITIAL_TIMER_TABLE init.script_seed
run_mutant m2 -DW34N90_MUTANT_ADVANCE_INITIAL_INDEX init.script_seed
run_mutant m3 -DW34N90_MUTANT_EXPIRE_TIMER_AT_ZERO timer.zero_is_live
run_mutant m4 -DW34N90_MUTANT_WRONG_COMMAND2_CLAIM command2.claim
run_mutant m5 -DW34N90_MUTANT_SKIP_COMMAND5_SECOND_CLAIM command5.claims_state
run_mutant m6 -DW34N90_MUTANT_WRONG_COMMAND7_STATE command7.state
run_mutant m7 -DW34N90_MUTANT_WRONG_SOUND_RANGE command8.sounds
run_mutant m8 -DW34N90_MUTANT_SKIP_COMMAND64_EXIT command64.exit

echo "W34N90 MODE13 SCRIPTED CONTROL CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean; retail SHA $actual"
