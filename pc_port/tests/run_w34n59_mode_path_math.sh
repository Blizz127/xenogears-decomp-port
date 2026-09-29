#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n59_mode_path_math"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n59_mode_path_math_prod_test.c"
  "$ROOT/pc_port/src/world_map_helper_76858.c"
  "$ROOT/pc_port/src/world_map_helper_94154.c"
  "$ROOT/pc_port/src/world_map_helper_97070.c"
)

mkdir -p "$OUT"

check_slice() {
  local start="$1"
  local end="$2"
  local expected="$3"
  local skip=$((start - 0x8006FAF0))
  local count=$((end - start))
  local actual
  actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 skip="$skip" count="$count" status=none | sha256sum | awk '{print $1}')"
  if [[ "$actual" != "$expected" ]]; then
    echo "ERROR: retail slice 0x$(printf '%08X' "$start") SHA mismatch: $actual" >&2
    exit 1
  fi
}

check_slice 0x80076858 0x80076954 2e0f9e5ebbb78a734f16e8565b669194f7e8716346684578f6d7ccd40d17d5db
check_slice 0x80097070 0x80097244 4d0fbc4c927d74b78faee039c3996b038668f7909c4e60974ad6340451ab85d8
check_slice 0x80094154 0x800941C4 b36aa7c6085cff6f1e273998a83c96be927bfdfcee291f24e9706a05f66821f1

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
    sed -n '1,160p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N59_MUTANT_DROP_MIDDLE_BIAS blend.middle_bias
run_mutant m2 -DW34N59_MUTANT_SKIP_ZERO_GUARD decompose.zero_guard
run_mutant m3 -DW34N59_MUTANT_SWAP_FIRST_RATAN_ARGS decompose.first_ratan_args
run_mutant m4 -DW34N59_MUTANT_WRONG_BASE_SOURCE decompose.base_matrix
run_mutant m5 -DW34N59_MUTANT_SKIP_FINAL_NEGATION decompose.final_negation
run_mutant m6 -DW34N59_MUTANT_DISTANCE_USE_Y distance.planar_xz

echo "W34N59 MODE PATH MATH CERTIFICATE PASS: O0/O2/UBSan; M1-M6 detected; strict warnings clean"
