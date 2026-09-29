#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n87_mode12_lifecycle"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n87_mode12_lifecycle_prod_test.c"
  "$ROOT/pc_port/src/world_map_mode12_lifecycle.c"
)

mkdir -p "$OUT"

check_slice() {
  local start="$1"
  local stop="$2"
  local expected="$3"
  local actual
  actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
    skip=$((start - 0x8006FAF0)) count=$((stop - start)) status=none | \
    sha256sum | awk '{print $1}')"
  if [[ "$actual" != "$expected" ]]; then
    printf 'ERROR: retail slice 0x%x..0x%x SHA mismatch: %s\n' \
      "$start" "$stop" "$actual" >&2
    exit 1
  fi
}

check_slice 0x8007BF50 0x8007C260 \
  3b8a07c9f0591e66f49a240a0b481970732e504b640a899ab0dcf37ac453d627
check_slice 0x8007C260 0x8007C36C \
  c87cc08ed787bcf56a042bc397978352c58c33bd7260c493ac547333c7110113
check_slice 0x8007BF50 0x8007C36C \
  d5754947b95867cd6ecd9aa1d01f49d075fd37e01c7a4ca0e41d661a56bfefca

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

run_mutant m1 -DW34N87_MUTANT_WRONG_TRANSITION_ARG setup.transition_args
run_mutant m2 -DW34N87_MUTANT_WRONG_MODE_CONSTANT setup.state_values
run_mutant m3 -DW34N87_MUTANT_SWAP_OBJECT_GPU_A setup.stage_order
run_mutant m4 -DW34N87_MUTANT_WRONG_REGISTER_PAIR setup.registration_pairs
run_mutant m5 -DW34N87_MUTANT_SKIP_SOUND_LINK setup.sound_link
run_mutant m6 -DW34N87_MUTANT_SKIP_PALETTE_TAIL setup.stage_order
run_mutant m7 -DW34N87_MUTANT_SKIP_SEDS_FREE teardown.seds_free
run_mutant m8 -DW34N87_MUTANT_WRONG_TEARDOWN_STATE teardown.state_values

rg -A3 'case 0x8007BF50u:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_8007BF50\(\)'
rg -A3 'case 0x8007C260u:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_8007C260\(\)'
finish_seam="$(sed -n '/int wm_mode12_stage_second_wave_finish/,/}/p' \
  "$ROOT/pc_port/src/world_map_init.c")"
printf '%s\n' "$finish_seam" | rg -q 'wm_second_wave_poll\(\)'
printf '%s\n' "$finish_seam" | rg -q 'wm_80076954\(\)'
if rg -n '\(u32\)\(uintptr_t\)' \
  "$ROOT/pc_port/src/world_map_mode12_lifecycle.c" >/dev/null; then
  echo "ERROR: mode-12 lifecycle contains host-pointer truncation" >&2
  exit 1
fi

echo "W34N87 MODE12 LIFECYCLE CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
