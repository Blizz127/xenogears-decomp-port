#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n99_mode15_camera_control"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n99_mode15_camera_control_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7e450.c"
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

check_slice $((0x8007E450)) $((0x8007E4E4)) \
  1da9fd8cb77d1763c50e634cbb240d1ecf04ee38e2ff1f790af79f08b2b7f414
check_slice $((0x8007E4E4)) $((0x8007EBBC)) \
  bb1d11fe32c85e4d81f7b001c642454475e64f171312ec825bbbc07b4fc39965
check_slice $((0x8007E450)) $((0x8007EBBC)) \
  359e73b48329029a597c904e40c2d6a9dfce439ca173c73cbd53ab77adc1aa15

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

run_mutant m1 -DW34N99_MUTANT_WRONG_INIT_VIEW_Y init.controls
run_mutant m2 -DW34N99_MUTANT_LATCH1_WRONG_ANGLE latch1.camera_state
run_mutant m3 -DW34N99_MUTANT_SKIP_CAMERA_BUILD camera.generic_call
run_mutant m4 -DW34N99_MUTANT_STATE1_WRONG_DELTA state1.path_delta
run_mutant m5 -DW34N99_MUTANT_STATE3_SIGN_EXTEND_Y state3.position_delta
run_mutant m6 -DW34N99_MUTANT_STATE5_WRONG_CLAMP state5.clamp
run_mutant m7 -DW34N99_MUTANT_STATE24_WRONG_AMPLITUDE state24.transition
run_mutant m8 -DW34N99_MUTANT_SKIP_JITTER_MIRROR jitter.mirrored_offset

echo "W34N99 MODE15 CAMERA CONTROL CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
