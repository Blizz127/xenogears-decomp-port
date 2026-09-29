#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n80_mode12_linked_marker"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n80_mode12_linked_marker_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7d078.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"
source "$ROOT/pc_port/tests/lib/world_map_matched.sh"
COMMON+=("$(world_map_matched_obj "$OUT" func_8007D078)")

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x8007D078 - 0x8006FAF0)) \
  count=$((0x8007D228 - 0x8007D078)) status=none | \
  sha256sum | awk '{print $1}')"
expected="d2e3b0c9d411d82a765a51b5047c0901a5685c0f7c914b5daffdc5715917c86e"
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

run_mutant m4 -DW34N80_MUTANT_SKIP_HEIGHT_BIAS move.height_bias
run_mutant m5 -DW34N80_MUTANT_WRONG_MARKER_ID move.marker
run_mutant m6 -DW34N80_MUTANT_SKIP_COMPLETION_FLAGS complete.flags
run_mutant m7 -DW34N80_MUTANT_WRONG_COMPLETION_RETURN complete.return

echo "W34N80 MODE12 LINKED MARKER CERTIFICATE PASS: O0/O2/UBSan; M4-M7 detected (M1-M3 targeted the retired wm_8007D078; 0x8007D078 now runs the matched C); strict warnings clean"
