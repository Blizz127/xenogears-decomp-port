#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n125_vertical_states"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n125_vertical_states_prod_test.c"
  "$ROOT/pc_port/src/world_map_state_vertical_8f690.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x8008F690 - 0x8006FAF0)) \
  count=$((0x8008F9D8 - 0x8008F690)) status=none | \
  sha256sum | awk '{print $1}')"
expected="3923239ef90bf80128093802ee58f13ed60b1022c656900e53c6b81ce230efbc"
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
    sed -n '1,200p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined


run_mutant m1 -DW34N125_MUTANT_M1_WRONG_BANK s8.bank
run_mutant m2 -DW34N125_MUTANT_M2_NO_GROUND_CLAMP s16.clamp
run_mutant m3 -DW34N125_MUTANT_M3_SKIP_HEIGHT_PUBLISH s16.height_publish
run_mutant m4 -DW34N125_MUTANT_M4_PARTY_ORDER s16.party_order

echo "W34N125 VERTICAL STATES CERTIFICATE PASS: O0/O2/UBSan; M1-M4 detected; strict warnings clean"
