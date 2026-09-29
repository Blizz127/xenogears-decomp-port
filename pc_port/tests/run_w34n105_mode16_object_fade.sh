#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n105_mode16_object_fade"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n105_mode16_object_fade_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_817a0.c"
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

check_slice $((0x800816DC)) $((0x800817A0)) \
  0f83d78059f92926b4e908f3ae687aaaa0da51239fd71e8fcaba07ea18f34b18
check_slice $((0x800817A0)) $((0x80081868)) \
  6f4bba9eb5e5535d43b2f0361319cac26f1035ecc8c760fdb73a9a05bdef0c7a
check_slice $((0x80081868)) $((0x800819C8)) \
  e2a6561950d1696cadd6e0ce5ba90866ccb8143913205be0bd6740d94e1ec540
check_slice $((0x800816DC)) $((0x800819C8)) \
  c4547102a4cbed6aa366ecd5c1d496581b678af45def85de04b7cd3f72247b3e

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

run_mutant m1 -DW34N105_MUTANT_WRONG_PRIMITIVE_CODE helper.packet_layout
run_mutant m2 -DW34N105_MUTANT_WRONG_TPAGE_X helper.tpage_args
run_mutant m3 -DW34N105_MUTANT_REVERSE_STREAM_COPY helper.copy_direction
run_mutant m4 -DW34N105_MUTANT_WRONG_INITIAL_Y init.position
run_mutant m5 -DW34N105_MUTANT_LOGICAL_Y_SHIFT init.position_publish
run_mutant m6 -DW34N105_MUTANT_LATCH1_STAYS_IDLE update.latch1
run_mutant m7 -DW34N105_MUTANT_KEEP_SEMITRANS_BIT update.latch2_restore
run_mutant m8 -DW34N105_MUTANT_WRONG_FADE_LIMIT update.fade_saturation

echo "W34N105 MODE16 OBJECT FADE CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
