#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n108_mode16_width_control"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n108_mode16_width_control_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_81fb4.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"
source "$ROOT/pc_port/tests/lib/world_map_matched.sh"
COMMON+=("$(world_map_matched_obj "$OUT" func_80081FB4)")

check_slice() {
  local lo="$1"
  local hi="$2"
  local expected="$3"
  local actual
  actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
    skip=$((lo - 0x8006FAF0)) count=$((hi - lo)) status=none | \
    sha256sum | awk '{print $1}')"
  if [[ "$actual" != "$expected" ]]; then
    printf 'ERROR: retail slice 0x%08X..0x%08X SHA mismatch: %s\n' \
      "$lo" "$hi" "$actual" >&2
    exit 1
  fi
}

check_slice $((0x80081FB4)) $((0x80081FD8)) \
  0e39044d68e3c1baa81845a86797cac09e7e749e38436b362dc2c417acc5e9ca
check_slice $((0x80081FD8)) $((0x80082324)) \
  489182ddd290227f2577f67778abfd1bd6d079068007f2d1e79482bb25da11c4
check_slice $((0x80081FB4)) $((0x80082324)) \
  7151eab77895432a0219d46eca8d551471bcd35435cba019b897be40c3b2b9b6

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
    sed -n '1,300p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m3 -DW34N108_MUTANT_KEEP_COMMAND command.consume_and_map
run_mutant m4 -DW34N108_MUTANT_WRONG_COMMAND_MAP command.consume_and_map
run_mutant m5 -DW34N108_MUTANT_SKIP_STATE0_FILL state0.fill_and_bands
run_mutant m6 -DW34N108_MUTANT_WRONG_GROW_CLAMP state1.grow_clamp_fill
run_mutant m7 -DW34N108_MUTANT_WRONG_SHRINK_CLAMP state2.shrink_clamp_fill
run_mutant m8 -DW34N108_MUTANT_WRONG_RANDOM_GATE state3.sparse_randomization
run_mutant m9 -DW34N108_MUTANT_WRONG_CONSTANT_FILL state4.constant_fill

echo "W34N108 MODE16 WIDTH CONTROL CERTIFICATE PASS: O0/O2/UBSan; M3-M9 detected; strict warnings clean; M1-M2 targeted the retired wm_80081FB4, which now runs the matched C"
