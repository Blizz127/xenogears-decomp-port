#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n98_mode15_scripted_control"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n98_mode15_scripted_control_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7de14.c"
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

check_slice $((0x8007DE14)) $((0x8007DE98)) \
  de90c2d19c4980b2e0f8804eb9a64f1bd34764050ae403cddd3174f2d88ea7fd
check_slice $((0x8007DE98)) $((0x8007E450)) \
  5d88d3981d5b63308ad174fbaee290844382ede697a9406ae118e6a6dce177c3
check_slice $((0x8007DE14)) $((0x8007E450)) \
  190f652180f4815f909b634f4fe3e874eda5b94f092c79ed6b7b9084e430072b
check_slice $((0x80089514)) $((0x80089580)) \
  6c1b35db616141b09065762f7dc2cc998494e0dfd03a336a30c7ad58a4553550

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

run_mutant m1 -DW34N98_MUTANT_WRONG_INITIAL_TIMER_TABLE init.script_seed
run_mutant m2 -DW34N98_MUTANT_KEEP_INITIAL_INDEX_ZERO init.script_seed
run_mutant m3 -DW34N98_MUTANT_EXPIRE_TIMER_AT_ZERO timer.zero_is_live
run_mutant m4 -DW34N98_MUTANT_COMMAND2_WRONG_FINAL_CLAIM command2.sound_claims
run_mutant m5 -DW34N98_MUTANT_SKIP_PARTICLE_CLEAR command3.particle_release
run_mutant m6 -DW34N98_MUTANT_COMMAND10_WRONG_GLOBALS command10.claims_state
run_mutant m7 -DW34N98_MUTANT_SOUND_TABLE_WRONG_STRIDE command61.sound_table
run_mutant m8 -DW34N98_MUTANT_SKIP_COMMAND64_EXIT command64.exit

echo "W34N98 MODE15 SCRIPTED CONTROL CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
