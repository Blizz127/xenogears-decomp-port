#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n102_mode16_lifecycle"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n102_mode16_lifecycle_prod_test.c"
  "$ROOT/pc_port/src/world_map_mode16_lifecycle.c"
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

check_slice $((0x80080D00)) $((0x8008106C)) \
  56b693c92cec3c3ad36ab58f0c94e98d718f91d7b699396b0289e0c8cbdef6da
check_slice $((0x8008106C)) $((0x80081174)) \
  3d75170cc44b97a7b11cfaa4b14310794e4a4e7370d19f57410ed2f060506337
check_slice $((0x80080D00)) $((0x80081174)) \
  daa11d626943c7d8ed410c704cbd69ef15a820604c2a11fbe68d2847fefa7677

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

run_mutant m1 -DW34N102_MUTANT_WRONG_TRANSITION_ARG setup.transition_args
run_mutant m2 -DW34N102_MUTANT_WRONG_MODE_CONSTANT setup.state_values
run_mutant m3 -DW34N102_MUTANT_SWAP_OBJECT_GPU_A setup.stage_order
run_mutant m4 -DW34N102_MUTANT_WRONG_REGISTER_PAIR setup.registration_pairs
run_mutant m5 -DW34N102_MUTANT_SKIP_WDS_LOAD setup.wds_load
run_mutant m6 -DW34N102_MUTANT_SKIP_SONG_START setup.song_start
run_mutant m7 -DW34N102_MUTANT_SKIP_SEDS_FREE teardown.seds_free
run_mutant m8 -DW34N102_MUTANT_WRONG_TEARDOWN_STATE teardown.state_values

rg -A3 'case 0x80080D00u:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_80080D00\(\)'
rg -A3 'case 0x8008106Cu:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_8008106C\(\)'
finish_seam="$(sed -n '/int wm_mode16_stage_second_wave_finish/,/}/p' \
  "$ROOT/pc_port/src/world_map_init.c")"
printf '%s\n' "$finish_seam" | rg -q 'wm_second_wave_poll\(\)'
printf '%s\n' "$finish_seam" | rg -q 'wm_80076954\(\)'
if rg -n '\(u32\)\(uintptr_t\)' \
  "$ROOT/pc_port/src/world_map_mode16_lifecycle.c" >/dev/null; then
  echo "ERROR: mode-16 lifecycle contains host-pointer truncation" >&2
  exit 1
fi

echo "W34N102 MODE16 LIFECYCLE CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
