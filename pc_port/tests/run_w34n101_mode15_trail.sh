#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n101_mode15_trail"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n101_mode15_trail_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7f8ac.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

check_slice() {
  local start="$1"
  local end="$2"
  local expected="$3"
  local actual
  actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
    skip=$((start - 0x8006FAF0)) count=$((end - start)) status=none | \
    sha256sum | awk '{print $1}')"
  if [[ "$actual" != "$expected" ]]; then
    printf 'ERROR: retail slice 0x%x..0x%x SHA mismatch: %s\n' \
      "$start" "$end" "$actual" >&2
    exit 1
  fi
}

check_slice $((0x8007F8AC)) $((0x8007F968)) \
  7340d6c133833a2f36ac3ebeaf2971119152dc239628ce27d93a8dfc86e1b544
check_slice $((0x8007F968)) $((0x8007FC8C)) \
  633a11cbaf8d636c831212113ecc00e28b73361ac329df28087ade7b4459f80c
check_slice $((0x8007F8AC)) $((0x8007FC8C)) \
  27ca49dbec1b55e2790f98d62a710a86b3c8ff2cb0a92dfc0d845ab0815fb5b8

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
    sed -n '1,200p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N101_MUTANT_WRONG_OWNER_STRIDE init.stream_owner
run_mutant m2 -DW34N101_MUTANT_WRONG_SLOT8_ABR init.slot8_abr
run_mutant m3 -DW34N101_MUTANT_WRONG_INITIAL_TIMER init.timer
run_mutant m4 -DW34N101_MUTANT_SKIP_LATCH2_STATE latch2.state
run_mutant m5 -DW34N101_MUTANT_WRONG_MARKER_OFFSET follow.marker_position
run_mutant m6 -DW34N101_MUTANT_WRONG_FADE_STEP fade.scale_step
run_mutant m7 -DW34N101_MUTANT_WRONG_FIXED_MATRIX render.fixed_matrix
run_mutant m8 -DW34N101_MUTANT_WRONG_SCALE_Z render.scale_vector

echo "W34N101 MODE15 TRAIL CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
