#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n106_mode16_dual_fade"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n106_mode16_dual_fade_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_819c8.c"
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

check_slice $((0x800819C8)) $((0x80081B24)) \
  d2bd501688fc74d3a383ac5784047a1b2333d1580a51524e3766bec9c64a7396
check_slice $((0x80081B24)) $((0x80081C3C)) \
  ce39af781dc1bdeaff8b117a059abc69cef2945e7c493402717cc74b106e635f
check_slice $((0x800819C8)) $((0x80081C3C)) \
  c6ba5b2910adea6da0c75d2840980017e99139474691045bcfd1700d70078784

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

run_mutant m1 -DW34N106_MUTANT_WRONG_INITIAL_Y init.position
run_mutant m2 -DW34N106_MUTANT_WRONG_SCALE_Y init.scale
run_mutant m3 -DW34N106_MUTANT_SKIP_MATRIX_MIRROR init.matrix_mirror
run_mutant m4 -DW34N106_MUTANT_WRONG_FIRST_ABR init.streams
run_mutant m5 -DW34N106_MUTANT_SKIP_SECOND_STREAM init.streams
run_mutant m6 -DW34N106_MUTANT_LATCH_STAYS_IDLE update.latch
run_mutant m7 -DW34N106_MUTANT_WRONG_FADE_STEPS update.fade_steps
run_mutant m8 -DW34N106_MUTANT_CLAMP_ALL_CHANNELS update.fade_saturation

echo "W34N106 MODE16 DUAL FADE CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
