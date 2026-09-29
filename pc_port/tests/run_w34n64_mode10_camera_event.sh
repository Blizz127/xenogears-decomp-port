#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n64_mode10_camera_event"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n64_mode10_camera_event_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_78e2c.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"
source "$ROOT/pc_port/tests/lib/world_map_matched.sh"
COMMON+=("$(world_map_matched_obj "$OUT" func_80078E2C)")

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x80078E2C - 0x8006FAF0)) \
  count=$((0x800794D8 - 0x80078E2C)) status=none | \
  sha256sum | awk '{print $1}')"
expected="9ee1f145bcf085ddff75248b9d2dbea1bb7d2261dbbdf12906725b3dab08e09c"
if [[ "$actual" != "$expected" ]]; then
  echo "ERROR: retail slice 0x80078E2C..0x800794D8 SHA mismatch: $actual" >&2
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

run_mutant m2 -DW34N64_MUTANT_SKIP_STATE0_CLAIM state0.claim
run_mutant m3 -DW34N64_MUTANT_SKIP_STATE2_LAST_CLAIM state2.claims
run_mutant m4 -DW34N64_MUTANT_WRONG_STATE4_MODULUS jitter.state4
run_mutant m5 -DW34N64_MUTANT_SKIP_MIRRORED_ANGLE jitter.state4
run_mutant m6 -DW34N64_MUTANT_WRONG_STATE6_GLOBALS state6.globals
run_mutant m7 -DW34N64_MUTANT_IGNORE_STATE16_FLAG entry.flag_gate
run_mutant m8 -DW34N64_MUTANT_WRONG_LOOP_RESET entry.loop_reset
run_mutant m9 -DW34N64_MUTANT_DOUBLE_VIEW_IN_JITTER_STATES jitter.view_once

echo "W34N64 MODE10 CAMERA/EVENT CERTIFICATE PASS: O0/O2/UBSan; M2-M9 detected (M1 targeted the retired wm_80078E2C; 0x80078E2C now runs the matched C); strict warnings clean"
