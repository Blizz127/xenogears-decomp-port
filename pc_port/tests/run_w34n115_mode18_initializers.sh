#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n115_mode18_initializers"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n115_mode18_initializers_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_838e8.c"
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

check_slice $((0x800838E8)) $((0x8008390C)) \
  f50927db5ce9246f63827545fa12f5c021b977e2f80f67309df70ca054e0c2e7
check_slice $((0x8008390C)) $((0x80083A00)) \
  8bbdad2839313cbef0510f1695a8dcd3cf127107fd747ade4555ae7cc09832c8
check_slice $((0x80083FE4)) $((0x80084068)) \
  b25fedbe29dc220a7607e784ff19e8afcc4e54a6474991699d2d6f1a97b4ba78

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

run_mutant m1 -DW34N115_MUTANT_WRONG_STATIC_RECORD init_static.record_pointer
run_mutant m2 -DW34N115_MUTANT_WRONG_SCALE camera_init.constants
run_mutant m3 -DW34N115_MUTANT_SKIP_POSITION_FANOUT camera_init.position_fanout
run_mutant m4 -DW34N115_MUTANT_WRONG_ANGLES camera_init.angles
run_mutant m5 -DW34N115_MUTANT_WRONG_CAMERA_HEIGHT camera_init.height
run_mutant m6 -DW34N115_MUTANT_WRONG_OBJECT_Z object_init.slot_position
run_mutant m7 -DW34N115_MUTANT_SKIP_OBJECT_ENABLE object_init.enable
run_mutant m8 -DW34N115_MUTANT_SKIP_OBJECT_TRANSLATION object_init.translation

for address in 800838E8 8008390C 80083FE4; do
  rg -A3 "guest_addr == 0x${address}u" \
    "$ROOT/pc_port/src/world_map_scheduler.c" |
    rg -q "wm_sched_builtin_${address}"
done

echo "W34N115 MODE18 INITIALIZERS CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
