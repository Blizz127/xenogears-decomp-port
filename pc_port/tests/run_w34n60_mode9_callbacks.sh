#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n60_mode9_callbacks"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n60_mode9_callbacks_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_77dc8.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

slice_actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 skip=$((0x80077DC8 - 0x8006FAF0)) count=$((0x80078948 - 0x80077DC8)) status=none | sha256sum | awk '{print $1}')"
slice_expected="c43efadac3b27b4f4d702a2e88f6ad57a9e1fe00f461002640460395b3e9e417"
if [[ "$slice_actual" != "$slice_expected" ]]; then
  echo "ERROR: retail slice 0x80077DC8..0x80078948 SHA mismatch: $slice_actual" >&2
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
    sed -n '1,180p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N60_MUTANT_WRONG_RENDER_Z context.render_z
run_mutant m2 -DW34N60_MUTANT_WORD_SOUND_VECTOR path.sound_vectors
run_mutant m3 -DW34N60_MUTANT_SKIP_TERMINAL_EXIT path.terminal_exit
run_mutant m4 -DW34N60_MUTANT_SKIP_LAST_LINK context.link_count
run_mutant m5 -DW34N60_MUTANT_WRONG_MATRIX_SOURCE context.matrix_d

echo "W34N60 MODE9 CALLBACK CERTIFICATE PASS: O0/O2/UBSan; M1-M5 detected; strict warnings clean"
