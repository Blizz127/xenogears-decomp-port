#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n110_mode17_transition"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n110_mode17_transition_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_834d0.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"
source "$ROOT/pc_port/tests/lib/world_map_matched.sh"
COMMON+=("$(world_map_matched_obj "$OUT" func_800834D0)")

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

check_slice $((0x800834D0)) $((0x800834D8)) \
  5ce5ad86d452c4d2422bd63e15223d5d6b3dfb77f224a88c1f476e9fb34e359d
check_slice $((0x800834D8)) $((0x8008355C)) \
  63db118754e342edcf4438e409d8e566c4a9bc36d38a117c1012e2b5aeb6d7c5
check_slice $((0x800834D0)) $((0x8008355C)) \
  49aa1e57f1db503db33c9b2c03221443fa32e1b62a663a5b300a25f6b882dd58

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

run_mutant m2 -DW34N110_MUTANT_WRONG_LATCH_VALUE update.latch_gate
run_mutant m3 -DW34N110_MUTANT_KEEP_LATCH update.latch_clear
run_mutant m4 -DW34N110_MUTANT_SKIP_RESET update.order
run_mutant m5 -DW34N110_MUTANT_WRONG_POSITION update.position
run_mutant m6 -DW34N110_MUTANT_SINGLE_DRAIN update.drain_loop

echo "W34N110 MODE17 TRANSITION CERTIFICATE PASS: O0/O2/UBSan; M2-M6 detected (M1 targeted the retired wm_800834D0; 0x800834D0 now runs the matched C); strict warnings clean"
