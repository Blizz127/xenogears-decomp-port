#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n117_mode18_object"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n117_mode18_object_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_84068.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x80084068 - 0x8006FAF0)) \
  count=$((0x8008440C - 0x80084068)) status=none | \
  sha256sum | awk '{print $1}')"
expected=85bdeac8df0c583a1cd43a66f14f180a34e49220d105461798bc56fe3d0c027b
if [[ "$actual" != "$expected" ]]; then
  echo "ERROR: retail 84068 slice SHA mismatch: $actual" >&2
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
    sed -n '1,300p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N117_MUTANT_KEEP_OBJECT_ENABLED latch1.object_disable
run_mutant m2 -DW34N117_MUTANT_LATCH2_WRONG_CLEAR latch2.clear_list
run_mutant m3 -DW34N117_MUTANT_LATCH3_WRONG_CLEAR latch3.clear_list
run_mutant m4 -DW34N117_MUTANT_LATCH4_WRONG_CLEAR latch4.clear_list
run_mutant m5 -DW34N117_MUTANT_WRONG_Y_STEP state1.interpolation
run_mutant m6 -DW34N117_MUTANT_STATE1_WRONG_LIST state1.presence_list
run_mutant m7 -DW34N117_MUTANT_STATE2_WRONG_LIST state2.presence_list
run_mutant m8 -DW34N117_MUTANT_STATE3_WRONG_LIST state3.presence_list
run_mutant m9 -DW34N117_MUTANT_STATE4_KEEP_Y state4.second_y_zero
run_mutant m10 -DW34N117_MUTANT_SKIP_OBJECT_PUBLISH object.position_publish

rg -A3 'guest_addr == 0x80084068u' \
  "$ROOT/pc_port/src/world_map_scheduler.c" | \
  rg -q 'wm_sched_builtin_80084068'

echo "W34N117 MODE18 OBJECT CERTIFICATE PASS: O0/O2/UBSan; M1-M10 detected; strict warnings clean"
