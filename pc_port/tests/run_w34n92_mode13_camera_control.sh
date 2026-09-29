#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n92_mode13_camera_control"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n92_mode13_camera_control_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_80578.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x80080578 - 0x8006FAF0)) \
  count=$((0x80080900 - 0x80080578)) status=none | \
  sha256sum | awk '{print $1}')"
expected="9c824ffe16c72b81eee6f15dd47134e2099919f34dc324c33cb15cb4ad05c189"
if [[ "$actual" != "$expected" ]]; then
  printf 'ERROR: retail slice SHA mismatch: %s\n' "$actual" >&2
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
    sed -n '1,240p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N92_MUTANT_WRONG_VIEW_HEIGHT init.camera_constants
run_mutant m2 -DW34N92_MUTANT_WRONG_LATCH1_PITCH latch1.state
run_mutant m3 -DW34N92_MUTANT_WRONG_PRIMARY_CLAMP state1.primary_clamp
run_mutant m4 -DW34N92_MUTANT_WRONG_PITCH_APPROACH_SHIFT state1.pitch_approach
run_mutant m5 -DW34N92_MUTANT_SKIP_GENERIC_CAMERA camera.generic_call
run_mutant m6 -DW34N92_MUTANT_WRONG_STATE3_CLAMP state3.clamp
run_mutant m7 -DW34N92_MUTANT_WRONG_JITTER_BIAS state1.jitter_mirror
run_mutant m8 -DW34N92_MUTANT_SKIP_JITTER_MIRROR state1.jitter_mirror

echo "W34N92 MODE13 CAMERA CONTROL CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
