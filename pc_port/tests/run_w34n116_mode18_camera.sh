#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n116_mode18_camera"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n116_mode18_camera_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_83a00.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x80083A00 - 0x8006FAF0)) \
  count=$((0x80083FE4 - 0x80083A00)) status=none | \
  sha256sum | awk '{print $1}')"
expected=5da7664c8dd74594161dbf6da5da01e60d419d1a73956589238c8f57818830a8
if [[ "$actual" != "$expected" ]]; then
  echo "ERROR: retail 83A00 slice SHA mismatch: $actual" >&2
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
    sed -n '1,280p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N116_MUTANT_LATCH1_WRONG_STATE latch.mapping
run_mutant m2 -DW34N116_MUTANT_LATCH2_WRONG_Y latch2.position
run_mutant m3 -DW34N116_MUTANT_SKIP_LATCH3_WORLD_X latch3.world_x
run_mutant m4 -DW34N116_MUTANT_LATCH4_WRONG_DRIFT latch4.world_drift
run_mutant m5 -DW34N116_MUTANT_SKIP_CAMERA_BUILD camera.call_contract
run_mutant m6 -DW34N116_MUTANT_STATE1_WRONG_STEP state1.interpolation
run_mutant m7 -DW34N116_MUTANT_STATE5_WRONG_TARGET state5.interpolation
run_mutant m8 -DW34N116_MUTANT_SKIP_MOTION_HELPERS motion.pipeline
run_mutant m9 -DW34N116_MUTANT_JITTER_NOT_CENTERED jitter.center

rg -A3 'guest_addr == 0x80083A00u' \
  "$ROOT/pc_port/src/world_map_scheduler.c" | \
  rg -q 'wm_sched_builtin_80083A00'

echo "W34N116 MODE18 CAMERA CERTIFICATE PASS: O0/O2/UBSan; M1-M9 detected; strict warnings clean"
