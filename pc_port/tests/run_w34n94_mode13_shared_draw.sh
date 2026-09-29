#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n94_mode13_shared_draw"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n94_mode13_shared_draw_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_76a14.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"
source "$ROOT/pc_port/tests/lib/world_map_matched.sh"
COMMON+=("$(world_map_matched_obj "$OUT" func_80076A14)")

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

check_slice $((0x80076A14)) $((0x80076A1C)) \
  5ce5ad86d452c4d2422bd63e15223d5d6b3dfb77f224a88c1f476e9fb34e359d
check_slice $((0x80076A1C)) $((0x80076B34)) \
  19a29eee50ec5ae67dd91bc44d9bcc464ba0189d52480eb0d4ce58f652487bb0
check_slice $((0x80076A14)) $((0x80076B34)) \
  fe05b5cdd6f6c88c87aca7429f5446e5c5131b675d2637c2130cb875c68e0e07

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

run_mutant m1 -DW34N94_MUTANT_SWAP_CAMERA_BRANCH no_paging.camera_branch
run_mutant m2 -DW34N94_MUTANT_SKIP_VERTEX_SETUP no_paging.vertex_setup
run_mutant m3 -DW34N94_MUTANT_SKIP_POSITION_WRAP no_paging.position_wrap
run_mutant m4 -DW34N94_MUTANT_SKIP_PAGING_CHAIN paging.chain_present
run_mutant m5 -DW34N94_MUTANT_WRONG_TERRAIN_POSITION paging.visibility_position
run_mutant m6 -DW34N94_MUTANT_WRONG_PALETTE_STEP no_paging.palette_step
run_mutant m7 -DW34N94_MUTANT_SKIP_TERRAIN_PACKETS no_paging.terrain_packets
run_mutant m8 -DW34N94_MUTANT_SKIP_FINAL_DRAW no_paging.final_draw

if rg -n '\(u32\)\(uintptr_t\)' \
  "$ROOT/pc_port/src/world_map_callback_76a14.c" >/dev/null; then
  echo "ERROR: mode-13 shared draw callback contains host-pointer truncation" >&2
  exit 1
fi

echo "W34N94 MODE13 SHARED DRAW CERTIFICATE PASS: O0/O2/UBSan; M1-M8 detected; strict warnings clean; 0x80076A14 now runs the matched C"
