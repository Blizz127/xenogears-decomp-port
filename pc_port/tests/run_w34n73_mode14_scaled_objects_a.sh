#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n73_mode14_scaled_objects_a"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n73_mode14_scaled_objects_a_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7b200.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x8007B200 - 0x8006FAF0)) \
  count=$((0x8007B604 - 0x8007B200)) status=none | \
  sha256sum | awk '{print $1}')"
expected="21bd3f79595e2d677c07d54cb77ec7570d1cbd623c3263faec1b730ab9a84de1"
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

run_mutant m1 -DW34N73_MUTANT_SKIP_SECOND_PRIMITIVE_INIT init.primitive_calls
run_mutant m2 -DW34N73_MUTANT_COUNT_FROM_PRIMITIVE_SOURCE init.primitive_arguments
run_mutant m3 -DW34N73_MUTANT_SKIP_SECOND_PHASE_SEED init.slot_seeds
run_mutant m4 -DW34N73_MUTANT_EARLY_SECONDARY_PHASE update.stagger_threshold
run_mutant m5 -DW34N73_MUTANT_WRONG_PHASE_CLAMP update.phase_clamps
run_mutant m6 -DW34N73_MUTANT_WRONG_SECOND_MATRIX_DESTINATION update.matrix_destinations

echo "W34N73 MODE14 SCALED OBJECTS A CERTIFICATE PASS: O0/O2/UBSan; M1-M6 detected; strict warnings clean"
