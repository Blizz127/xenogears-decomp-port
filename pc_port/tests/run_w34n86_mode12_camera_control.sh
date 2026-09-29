#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n86_mode12_camera_control"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n86_mode12_camera_control_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7c724.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x8007C724 - 0x8006FAF0)) \
  count=$((0x8007CC6C - 0x8007C724)) status=none | \
  sha256sum | awk '{print $1}')"
expected="b2feeed8f84d242cbaaf14381c79c68eb698f4dac6f0979cfc0643170f94a878"
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
    sed -n '1,260p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N86_MUTANT_WRONG_INIT_VIEW_HEIGHT init.camera_constants
run_mutant m2 -DW34N86_MUTANT_WRONG_LATCH1_THRESHOLD latch.one
run_mutant m3 -DW34N86_MUTANT_WRONG_STATE2_CLAMP state2.negative_clamp
run_mutant m4 -DW34N86_MUTANT_WRONG_STATE4_MARKER state4.markers
run_mutant m5 -DW34N86_MUTANT_WRONG_STATE6_TABLE state6.final_table
run_mutant m6 -DW34N86_MUTANT_SKIP_GENERIC_CAMERA camera.generic_gate
run_mutant m7 -DW34N86_MUTANT_WRONG_BLEND_REMAINDER camera.blend_args
run_mutant m8 -DW34N86_MUTANT_SKIP_JITTER_MIRROR jitter.mirrored_axes

echo "W34N86 MODE12 CAMERA CONTROL CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
