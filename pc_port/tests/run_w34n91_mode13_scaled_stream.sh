#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n91_mode13_scaled_stream"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n91_mode13_scaled_stream_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_80a28.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x800809EC - 0x8006FAF0)) \
  count=$((0x80080D00 - 0x800809EC)) status=none | \
  sha256sum | awk '{print $1}')"
expected="73c678706ea4d808fdccb738e0450671a63a8ee4491b3e0a04d0442a2e0a3991"
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

run_mutant m1 -DW34N91_MUTANT_WRONG_INITIAL_Y init.position
run_mutant m2 -DW34N91_MUTANT_SKIP_SECOND_STREAM init.streams
run_mutant m3 -DW34N91_MUTANT_SKIP_LATCH_CLEAR update.latch
run_mutant m4 -DW34N91_MUTANT_WRONG_POSITION_SHIFT update.position
run_mutant m5 -DW34N91_MUTANT_SWAP_SCALE_VECTORS update.scale_order
run_mutant m6 -DW34N91_MUTANT_WRONG_PHASE_STEP update.phase_steps
run_mutant m7 -DW34N91_MUTANT_WRONG_STREAM_SIDE update.stream_side
run_mutant m8 -DW34N91_MUTANT_WRONG_FADE_STEP update.fade_step

echo "W34N91 MODE13 SCALED STREAM CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
