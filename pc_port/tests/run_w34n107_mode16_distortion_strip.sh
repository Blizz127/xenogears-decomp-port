#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n107_mode16_distortion_strip"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n107_mode16_distortion_strip_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_81c3c.c"
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

check_slice $((0x80081C3C)) $((0x80081D80)) \
  be3a3d2145779e0491bae8222e8fa7e106cd06f072926f7f2f32a48e9d1d9f85
check_slice $((0x80081D80)) $((0x80081FB4)) \
  5afe7efefdd15e45ed722657d3d3de04d962448b11ae2234ea83153c539470d3
check_slice $((0x80081C3C)) $((0x80081FB4)) \
  157d1127b1e5d3dafa1f47894557070b89000a38ddb8171eaede5d57b6126566

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

run_mutant m1 -DW34N107_MUTANT_RAW_HOST_POINTER init.alloc_publish
run_mutant m2 -DW34N107_MUTANT_WRONG_ALLOC_SIZE init.alloc_sizes
run_mutant m3 -DW34N107_MUTANT_WRONG_PRIMITIVE_CODE init.primitive_layout
run_mutant m4 -DW34N107_MUTANT_SKIP_BUFFER_MIRROR init.buffer_mirror
run_mutant m5 -DW34N107_MUTANT_WRONG_WIDTH_SEED init.width_seed
run_mutant m6 -DW34N107_MUTANT_WRONG_STREAM_SIDE update.selected_buffer
run_mutant m7 -DW34N107_MUTANT_SKIP_WIDTH_CENTERING update.strip_geometry
run_mutant m8 -DW34N107_MUTANT_WRONG_MOVE_DESTINATION update.move_packet

echo "W34N107 MODE16 DISTORTION STRIP CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
