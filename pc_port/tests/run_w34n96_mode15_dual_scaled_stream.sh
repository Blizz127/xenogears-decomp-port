#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n96_mode15_dual_scaled_stream"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n96_mode15_dual_scaled_stream_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7fc8c.c"
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

check_slice 0x8007FC8C 0x8007FD30 \
  f80cf59d17e4d380916903078ae6408fb1895db691f338ae4b2d2686d1e37364
check_slice 0x8007FD30 0x8007FF70 \
  b837651d395e719555143f5b6cf448eb1e3078a88331bd687548724f9e28a52b
check_slice 0x8007FC8C 0x8007FF70 \
  2388f5f12744842aee11edd62a84c7025f12785a4d6901163bc7d3284afa0cbe

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

run_mutant m1 -DW34N96_MUTANT_WRONG_INITIAL_X init.position
run_mutant m2 -DW34N96_MUTANT_SKIP_SECOND_STREAM init.streams
run_mutant m3 -DW34N96_MUTANT_SKIP_LATCH_CLEAR update.latch
run_mutant m4 -DW34N96_MUTANT_WRONG_POSITION_SHIFT update.position
run_mutant m5 -DW34N96_MUTANT_SWAP_SCALE_VECTORS update.scale_order
run_mutant m6 -DW34N96_MUTANT_WRONG_PHASE_STEP update.phase_steps
run_mutant m7 -DW34N96_MUTANT_WRONG_STREAM_SIDE update.stream_side
run_mutant m8 -DW34N96_MUTANT_WRONG_FADE_STEP update.fade_step

echo "W34N96 MODE15 DUAL SCALED STREAM CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
