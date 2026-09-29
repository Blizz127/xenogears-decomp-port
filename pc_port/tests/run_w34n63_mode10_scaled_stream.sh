#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n63_mode10_scaled_stream"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n63_mode10_scaled_stream_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7a144.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x8007A06C - 0x8006FAF0)) \
  count=$((0x8007A410 - 0x8007A06C)) status=none | \
  sha256sum | awk '{print $1}')"
expected="b6813757c0399adc9e19b760c1388de87eb4effa191fa69b9c28dfeb70f30707"
if [[ "$actual" != "$expected" ]]; then
  echo "ERROR: retail slice 0x8007A06C..0x8007A410 SHA mismatch: $actual" >&2
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
    sed -n '1,200p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N63_MUTANT_WRONG_PRIMITIVE_CODE stream.primitive_code
run_mutant m2 -DW34N63_MUTANT_REVERSE_STREAM_COPY stream.copy_direction
run_mutant m3 -DW34N63_MUTANT_SKIP_SECOND_STREAM init.second_stream
run_mutant m4 -DW34N63_MUTANT_WRONG_PRIMARY_STEP update.primary_phase
run_mutant m5 -DW34N63_MUTANT_LATE_TERMINAL_THRESHOLD update.terminal
run_mutant m6 -DW34N63_MUTANT_SWAP_SCALE_VECTORS update.scale_order
run_mutant m7 -DW34N63_MUTANT_SHORT_MATRIX_COPY update.matrix_copy

echo "W34N63 MODE10 SCALED-STREAM CERTIFICATE PASS: O0/O2/UBSan; M1-M7 detected; strict warnings clean"
