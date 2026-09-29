#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n57_mode811_lifecycle"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n57_mode811_lifecycle_prod_test.c"
  "$ROOT/pc_port/src/world_map_mode811_lifecycle.c"
)

mkdir -p "$OUT"

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
    cat "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N57_MUTANT_SWAP_OBJECT_GPU_A setup.stage_order
run_mutant m2 -DW34N57_MUTANT_WRONG_REGISTER_PAIR setup.registration_pairs
run_mutant m3 -DW34N57_MUTANT_DRAIN_GE_TWO setup.cd_drain_positive
run_mutant m4 -DW34N57_MUTANT_SKIP_C180_FREE teardown.free_sequence
run_mutant m5 -DW34N57_MUTANT_WRONG_TEARDOWN_STATE teardown.state_values

rg -A3 'case 0x80077214u:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_80077214\(\)'
rg -A3 'case 0x80077480u:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_80077480\(\)'
rg -A2 'slot0 == 0x80071EF0u' "$ROOT/pc_port/src/world_map_init.c" |
  rg -q 'wm_mode811_stage_second_wave_submit\(\)'
submit_seam="$(sed -n '/int wm_mode811_stage_second_wave_submit/,/}/p' \
  "$ROOT/pc_port/src/world_map_init.c")"
finish_seam="$(sed -n '/int wm_mode811_stage_second_wave_finish/,/}/p' \
  "$ROOT/pc_port/src/world_map_init.c")"
printf '%s\n' "$submit_seam" | rg -q 'wm_80071EF0_second_wave\(\)'
printf '%s\n' "$finish_seam" | rg -q 'wm_second_wave_poll\(\)'
printf '%s\n' "$finish_seam" | rg -q 'wm_80073530_fixup\(\)'
if rg -n '\(u32\)\(uintptr_t\)' \
  "$ROOT/pc_port/src/world_map_mode811_lifecycle.c" >/dev/null; then
  echo "ERROR: lifecycle contains a host-pointer truncation" >&2
  exit 1
fi
if rg -n 'W10B did not run \(required\)|W12B did not run \(required\)|W14B did not run \(required\)|W17B did not run' \
  "$ROOT/pc_port/src/world_map_init.c" >/dev/null; then
  echo "ERROR: a base-only stage guard still blocks the retail mode-8/11 order" >&2
  exit 1
fi

echo "W34N57 MODE8/11 LIFECYCLE CERTIFICATE PASS: O0/O2/UBSan; M1-M5 detected; strict warnings clean"
