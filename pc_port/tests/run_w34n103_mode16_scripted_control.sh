#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n103_mode16_scripted_control"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n103_mode16_scripted_control_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_81174.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

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

check_slice $((0x80081174)) $((0x800811C0)) \
  864390670113403bf12e13be0047e28317ecfbac4c07f6af96e08ff8944f514c
check_slice $((0x800811C0)) $((0x800813E8)) \
  3028b49e6f82656e76562451f790b900c56c0217d1be9898426db7ebc87a3824
check_slice $((0x80081174)) $((0x800813E8)) \
  ee425beae1da78b70b258628ecd08e3afce792fa842d91dd8e0d11208a58e984

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
    sed -n '1,220p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N103_MUTANT_WRONG_INITIAL_TIMER_TABLE init.script_seed
run_mutant m2 -DW34N103_MUTANT_SKIP_INITIAL_INDEX_ADVANCE init.script_seed
run_mutant m3 -DW34N103_MUTANT_EXPIRE_TIMER_AT_ZERO timer.zero_is_live
run_mutant m4 -DW34N103_MUTANT_WRONG_COMMAND3_CLAIM command3.claim
run_mutant m5 -DW34N103_MUTANT_WRONG_SOUND_RANGE command8.sounds
run_mutant m6 -DW34N103_MUTANT_SKIP_COMMAND16_SECOND_CLAIM command16.claims
run_mutant m7 -DW34N103_MUTANT_WRONG_COMMAND63_FADE command63.fade_state
run_mutant m8 -DW34N103_MUTANT_SKIP_COMMAND64_EXIT command64.exit

echo "W34N103 MODE16 SCRIPTED CONTROL CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
