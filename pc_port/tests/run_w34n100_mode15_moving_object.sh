#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n100_mode15_moving_object"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n100_mode15_moving_object_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_7eca4.c"
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

check_slice $((0x8007EBBC)) $((0x8007ECA4)) \
  30a467d29779dcc703194884efaa60f5cffec04d35b321655d5fcd26b223820d
check_slice $((0x8007ECA4)) $((0x8007EE34)) \
  7ddcfde63cd20a06e97c0053d0cb60fe35383d9068c85f73d21661576a159d91
check_slice $((0x8007EE34)) $((0x8007F8AC)) \
  4e618ec4fb4925b9a99a9277adf8d1446426cfb08f93a669bb67681a1abda850
check_slice $((0x8007EBBC)) $((0x8007F8AC)) \
  a037a9c4198e9bddd4de4aeebc5911d6372b64bf929bdce98c45bfaaf5dcc7c0

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

run_mutant m1 -DW34N100_MUTANT_WRONG_TPAGE primitive.tpage_args
run_mutant m2 -DW34N100_MUTANT_SKIP_THIRD_LINK init.context_links
run_mutant m3 -DW34N100_MUTANT_WRONG_TARGET_STRIDE init.target_table
run_mutant m4 -DW34N100_MUTANT_WRONG_INIT_DISTANCE init.position_distance
run_mutant m5 -DW34N100_MUTANT_WRONG_STATE1_LIMIT state1.strict_limit
run_mutant m6 -DW34N100_MUTANT_SKIP_CLAIM_RANGE state0.claim_range
run_mutant m7 -DW34N100_MUTANT_WRONG_FIXED_MATRIX render.fixed_matrix
run_mutant m8 -DW34N100_MUTANT_SKIP_EFFECT_RELEASE state65.release_return

echo "W34N100 MODE15 MOVING OBJECT CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean"
