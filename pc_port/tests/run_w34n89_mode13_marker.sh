#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n89_mode13_marker"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n89_mode13_marker_prod_test.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"
source "$ROOT/pc_port/tests/lib/world_map_matched.sh"
COMMON+=("$(world_map_matched_obj "$OUT" func_80080900 func_80080944)")

actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
  skip=$((0x80080900 - 0x8006FAF0)) \
  count=$((0x800809EC - 0x80080900)) status=none | \
  sha256sum | awk '{print $1}')"
expected="195e8b7fde30969a64b3014c3f7e030f73a331d5af6fdac2c577809bb9b04bf4"
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
    sed -n '1,200p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined


echo "W34N89 MODE13 MARKER CERTIFICATE PASS: O0/O2/UBSan; strict warnings clean (both 0x80080900 and 0x80080944 now run the matched C, so the mutants of the retired wm_ bodies are gone)"
