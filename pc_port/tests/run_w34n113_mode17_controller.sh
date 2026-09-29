#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n113_mode17_controller"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n113_mode17_controller_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_827ec.c"
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

check_slice $((0x80076DA4)) $((0x80076F54)) \
  9ca630c02dbc139b19922610562d62cd67869a1c38f2e6f1fcb61ca09934f008
check_slice $((0x80076F54)) $((0x80076FA8)) \
  d6eb171c02b2c15cbe00e6f89e2ab5031493f1035e97c8c62a48b8ea69509f8d
check_slice $((0x80076FA8)) $((0x800771D8)) \
  2ef8c2f88eb7c643e72b0592c731e9358b30bd8e18f7174d1c1bce8d54e82ae7
check_slice $((0x800771D8)) $((0x80077214)) \
  6fa6ffda1e1b785f1fa51ecb300e546a55368e90c480e8abcea01a0620219f35
check_slice $((0x800827EC)) $((0x800828DC)) \
  099342952f8dd1fa9410b40a16350c7e5d3df5829ac986b60c500a383edef568
check_slice $((0x800828DC)) $((0x80082F64)) \
  355ca7f913e39472888eab6375a32ae92a1947fb36f5c6a417cdc8a15cf4a7e3
check_slice $((0x800827EC)) $((0x80082F64)) \
  8886a3c010c021bcb0fad54a3809a7647c304bb94567f407d961bbc90fe70765

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
    sed -n '1,360p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N113_MUTANT_WRONG_INITIAL_ANGLE init.camera_state
run_mutant m2 -DW34N113_MUTANT_WRONG_VIEW_HEIGHT init.constants
run_mutant m3 -DW34N113_MUTANT_APPROACH_OVERSHOOTS approach.strict_clamp
run_mutant m4 -DW34N113_MUTANT_LATCH8_WRONG_STATE latch.mapping
run_mutant m5 -DW34N113_MUTANT_WRONG_LATCH12_STATE latch.mapping
run_mutant m6 -DW34N113_MUTANT_SKIP_CAMERA_BUILD camera.call_contract
run_mutant m7 -DW34N113_MUTANT_SCALE_CLAMPS_EARLY state4.strict_ceiling
run_mutant m8 -DW34N113_MUTANT_ROTATION_THRESHOLD_512 motion.rotation_step
run_mutant m9 -DW34N113_MUTANT_POSITION_THRESHOLD_64 motion.position_step
run_mutant m10 -DW34N113_MUTANT_STATE2_WRONG_STEP_SIGN state2.step_direction
run_mutant m11 -DW34N113_MUTANT_SKIP_STATE11_WRAP state11.wrap
run_mutant m12 -DW34N113_MUTANT_JITTER_NOT_CENTERED jitter.center
run_mutant m13 -DW34N113_MUTANT_SKIP_MOTION_HELPERS motion.pipeline

echo "W34N113 MODE17 CONTROLLER CERTIFICATE PASS: O0/O2/UBSan; M1-M13 detected; strict warnings clean"
