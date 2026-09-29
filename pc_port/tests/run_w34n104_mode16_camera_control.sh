#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n104_mode16_camera_control"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n104_mode16_camera_control_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_813e8.c"
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

check_slice $((0x800813E8)) $((0x80081470)) \
  da6c4f94b4c53a470378bc52a4b2fb46886389cb58b48869b2ea311ee9663852
check_slice $((0x80081470)) $((0x800816DC)) \
  b593b5251048726b272b474a91e0f16ae84f11c9e5da4f03b88490a5fa891a9e
check_slice $((0x800813E8)) $((0x800816DC)) \
  77763293e4eb6bcd696148ae5a3bbda5a85d8d9e537e29738022fad56c03c097

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

run_mutant m1 -DW34N104_MUTANT_WRONG_INIT_VIEW_HEIGHT init.camera_constants
run_mutant m2 -DW34N104_MUTANT_WRONG_LATCH1_PITCH latch1.state
run_mutant m3 -DW34N104_MUTANT_SKIP_GENERIC_CAMERA camera.generic_call
run_mutant m4 -DW34N104_MUTANT_WRONG_PRIMARY_CLAMP state1.primary_clamp
run_mutant m5 -DW34N104_MUTANT_WRONG_ROLL_APPROACH_SHIFT state1.roll_approach
run_mutant m6 -DW34N104_MUTANT_WRONG_PITCH_APPROACH_SHIFT state1.pitch_approach
run_mutant m7 -DW34N104_MUTANT_WRONG_JITTER_BIAS state1.jitter_mirror
run_mutant m8 -DW34N104_MUTANT_SKIP_JITTER_MIRROR state1.jitter_mirror

echo "W34N104 MODE16 CAMERA CONTROL CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
