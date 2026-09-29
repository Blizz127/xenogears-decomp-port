#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n65_mode10_sequence_context"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n65_mode10_sequence_context_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_795e4.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x800795E4 - 0x8006FAF0)) \
  count=$((0x8007A06C - 0x800795E4)) status=none | \
  sha256sum | awk '{print $1}')"
expected="5c7badd535b7a5167f2ce758add4cfea97931bfd130cd1136e8e2d2e27affec7"
if [[ "$actual" != "$expected" ]]; then
  echo "ERROR: retail slice 0x800795E4..0x8007A06C SHA mismatch: $actual" >&2
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
    sed -n '1,260p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N65_MUTANT_SKIP_LAST_LINK init.links
run_mutant m2 -DW34N65_MUTANT_WRONG_STATE0_THRESHOLD state0.transition
run_mutant m3 -DW34N65_MUTANT_WRONG_STATE1_ACCELERATION state1.motion
run_mutant m4 -DW34N65_MUTANT_SKIP_STATE2_CLAIM state2.claim
run_mutant m5 -DW34N65_MUTANT_SKIP_STATE3_ADVANCE state3.transition
run_mutant m6 -DW34N65_MUTANT_SHORT_STATE4_FLAGS state4.flags
run_mutant m7 -DW34N65_MUTANT_SKIP_WRAP tail.wrap_order
run_mutant m8 -DW34N65_MUTANT_WRONG_PHASE_MASK tail.phases
run_mutant m9 -DW34N65_MUTANT_WRONG_MATRIX_SOURCE tail.matrix_d_chain
run_mutant m10 -DW34N65_MUTANT_SKIP_VIEW_MATRIX tail.view_matrix
run_mutant m11 -DW34N65_MUTANT_REBUILD_SECOND_PRESENCE state0.presence_reuse

echo "W34N65 MODE10 SEQUENCE/CONTEXT CERTIFICATE PASS: O0/O2/UBSan; M1-M11 detected; strict warnings clean"
