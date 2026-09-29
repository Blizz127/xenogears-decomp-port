#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n85_mode12_scripted_control"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n85_mode12_scripted_control_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7c36c.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x8007C36C - 0x8006FAF0)) \
  count=$((0x8007C724 - 0x8007C36C)) status=none | \
  sha256sum | awk '{print $1}')"
expected="ee52abd571a48b474dbbe975d341cd1eaf874bd51b2074a4996b9246977e2c26"
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
    sed -n '1,240p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N85_MUTANT_WRONG_INITIAL_TIMER_TABLE init.script_seed
run_mutant m2 -DW34N85_MUTANT_EXPIRE_TIMER_AT_ZERO timer.zero_is_live
run_mutant m3 -DW34N85_MUTANT_WRONG_COMMAND2_CLAIM command2.claim
run_mutant m4 -DW34N85_MUTANT_WRONG_COMMAND16_MARKER command16.marker
run_mutant m5 -DW34N85_MUTANT_WRONG_COMMAND17_TIMER command17.globals
run_mutant m6 -DW34N85_MUTANT_SKIP_COMMAND18_SLOT7 command18.claims
run_mutant m7 -DW34N85_MUTANT_WRONG_COMMAND22_STATE command22.globals_state
run_mutant m8 -DW34N85_MUTANT_SKIP_COMMAND64_EXIT command64.exit

echo "W34N85 MODE12 SCRIPTED CONTROL CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
