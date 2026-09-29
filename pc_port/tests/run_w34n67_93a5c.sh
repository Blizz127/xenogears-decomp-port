#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n67_93a5c"
CC_BIN="${CC:-clang}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n67_93a5c_prod_test.c"
  "$ROOT/pc_port/src/world_map_helper_93a5c.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x80093A5C - 0x8006FAF0)) \
  count=$((0x80093E8C - 0x80093A5C)) status=none | \
  sha256sum | awk '{print $1}')"
expected="95e643f45cc9e23616b613d8ae1d66f423d5c36fa504463f362aff2530c9e8df"
if [[ "$actual" != "$expected" ]]; then
  echo "ERROR: retail slice 0x80093A5C..0x80093E8C SHA mismatch: $actual" >&2
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
  if ! rg -q "ASSERTION $assertion FAILED" "$OUT/$name.log"; then
    echo "ERROR: $name did not fail named assertion $assertion" >&2
    sed -n '1,220p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime O0 -O0 -g
run_regime O2 -O2
run_regime UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

run_mutant M1 -DW34N67_MUTANT_CELL_Z_FROM_X cell_lookup.coordinates
run_mutant M2 -DW34N67_MUTANT_NO_X_NEIGHBOR height.rcos_order
run_mutant M3 -DW34N67_MUTANT_WRONG_H01_BYTE height.corner_values
run_mutant M4 -DW34N67_MUTANT_INVERT_FLAG selection.edges
run_mutant M5 -DW34N67_MUTANT_SWAP_CROSS cross.operands
run_mutant M6 -DW34N67_MUTANT_POSITIVE_QUERY_Z plane.query
run_mutant M7 -DW34N67_MUTANT_BASE_ALWAYS_H00 plane.base
run_mutant M8 -DW34N67_MUTANT_RETURN_SHIFT_3 return.shift

if rg -n 'result stored somewhere|/\* \$a1 \*/|wm_80093660\(void\)' \
  "$ROOT/pc_port/src/world_map_helper_93a5c.c" >/dev/null; then
  echo "ERROR: retired placeholder transcription remains" >&2
  exit 1
fi

echo "W34N67 93A5C CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
