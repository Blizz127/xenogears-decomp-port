#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n70_mode14_timed_marker"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n70_mode14_timed_marker_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7ba08.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"
source "$ROOT/pc_port/tests/lib/world_map_matched.sh"
COMMON+=("$(world_map_matched_obj "$OUT" func_8007BA08)")

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x8007BA08 - 0x8006FAF0)) \
  count=$((0x8007BB60 - 0x8007BA08)) status=none | \
  sha256sum | awk '{print $1}')"
expected="3337dc702544590b29ae15106982f4dc9821332e1773ff19a5553cc36bdbfa4e"
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
    sed -n '1,180p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m2 -DW34N70_MUTANT_WRONG_TIMER_SEED latch.timer_seed
run_mutant m3 -DW34N70_MUTANT_SKIP_X_STEP active.velocity
run_mutant m4 -DW34N70_MUTANT_WRONG_MARKER_DATA active.marker_arguments
run_mutant m5 -DW34N70_MUTANT_SKIP_RESET_HEADING expiry.reset_record

echo "W34N70 MODE14 TIMED MARKER CERTIFICATE PASS: O0/O2/UBSan; M2-M5 detected; strict warnings clean; M1 targeted the retired wm_8007BA08, which now runs the matched C"
