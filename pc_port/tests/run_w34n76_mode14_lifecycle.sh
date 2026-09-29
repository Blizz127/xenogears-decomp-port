#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n76_mode14_lifecycle"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n76_mode14_lifecycle_prod_test.c"
  "$ROOT/pc_port/src/world_map_mode14_lifecycle.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x8007A5DC - 0x8006FAF0)) \
  count=$((0x8007A9B4 - 0x8007A5DC)) status=none | \
  sha256sum | awk '{print $1}')"
expected="dcfb4937c169a6c1a38a9f794a11041b4249b22cb2074d81ce5569c3799eb723"
if [[ "$actual" != "$expected" ]]; then
  echo "ERROR: retail slice 0x8007A5DC..0x8007A9B4 SHA mismatch: $actual" >&2
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
    sed -n '1,220p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N76_MUTANT_WRONG_TRANSITION_ARG setup.transition_args
run_mutant m2 -DW34N76_MUTANT_WRONG_MODE_CONSTANT setup.state_values
run_mutant m3 -DW34N76_MUTANT_SWAP_OBJECT_GPU_A setup.stage_order
run_mutant m4 -DW34N76_MUTANT_WRONG_REGISTER_PAIR setup.registration_pairs
run_mutant m5 -DW34N76_MUTANT_SKIP_SOUND_LINK setup.sound_link
run_mutant m6 -DW34N76_MUTANT_SKIP_PALETTE_TAIL setup.stage_order
run_mutant m7 -DW34N76_MUTANT_SKIP_SEDS_FREE teardown.seds_free
run_mutant m8 -DW34N76_MUTANT_WRONG_TEARDOWN_STATE teardown.state_values

rg -A3 'case 0x8007A5DCu:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_8007A5DC\(\)'
rg -A3 'case 0x8007A8ACu:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_8007A8AC\(\)'
finish_seam="$(sed -n '/int wm_mode14_stage_second_wave_finish/,/}/p' \
  "$ROOT/pc_port/src/world_map_init.c")"
printf '%s\n' "$finish_seam" | rg -q 'wm_second_wave_poll\(\)'
printf '%s\n' "$finish_seam" | rg -q 'wm_80076954\(\)'
if rg -n '\(u32\)\(uintptr_t\)' \
  "$ROOT/pc_port/src/world_map_mode14_lifecycle.c" >/dev/null; then
  echo "ERROR: mode-14 lifecycle contains host-pointer truncation" >&2
  exit 1
fi

echo "W34N76 MODE14 LIFECYCLE CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
