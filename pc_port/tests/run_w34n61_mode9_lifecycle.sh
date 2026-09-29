#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n61_mode9_lifecycle"
CC_BIN="${CC:-cc}"
FLAGS=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
)
ARCHIVE_COMMON=(
  "${FLAGS[@]}"
  "$ROOT/pc_port/tests/w34n61_mode9_archive_prod_test.c"
  "$ROOT/pc_port/src/world_map_helper_76954.c"
)
LIFECYCLE_COMMON=(
  "${FLAGS[@]}"
  "$ROOT/pc_port/tests/w34n61_mode9_lifecycle_prod_test.c"
  "$ROOT/pc_port/src/world_map_mode9_lifecycle.c"
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
    echo "ERROR: retail slice 0x$(printf '%08X' "$start") SHA mismatch: $actual" >&2
    exit 1
  fi
}

check_slice 0x80076954 0x80076A14 89e5625f0d853113669915f9cb69f2d1dfbb017162d777e77541104478c730cc
check_slice 0x800721E4 0x80072238 1ae7931d6500b7844f35add116401df406500fc48076c21626ffcba05c5ba2d8
check_slice 0x80077A64 0x80077DC8 279c30f95bbb352733c5ccfa20041685f8324e68935ec08e8ab5c1b242f4b045

run_regime() {
  local family="$1"
  local name="$2"
  shift 2
  local -a common
  if [[ "$family" == archive ]]; then
    common=("${ARCHIVE_COMMON[@]}")
  else
    common=("${LIFECYCLE_COMMON[@]}")
  fi
  "$CC_BIN" "${common[@]}" "$@" -o "$OUT/${family}_${name}"
  "$OUT/${family}_${name}"
}

run_mutant() {
  local family="$1"
  local name="$2"
  local define="$3"
  local assertion="$4"
  local -a common
  if [[ "$family" == archive ]]; then
    common=("${ARCHIVE_COMMON[@]}")
  else
    common=("${LIFECYCLE_COMMON[@]}")
  fi
  "$CC_BIN" "${common[@]}" -O2 "$define" -o "$OUT/${family}_${name}"
  if "$OUT/${family}_${name}" >"$OUT/${family}_${name}.log" 2>&1; then
    echo "ERROR: ${family}_${name} survived" >&2
    exit 1
  fi
  if ! grep -q "ASSERTION $assertion FAILED" \
      "$OUT/${family}_${name}.log"; then
    echo "ERROR: ${family}_${name} did not fail $assertion" >&2
    sed -n '1,180p' "$OUT/${family}_${name}.log" >&2
    exit 1
  fi
  echo "${family}_${name} DETECTED ($assertion)"
}

for family in archive lifecycle; do
  run_regime "$family" o0 -O0
  run_regime "$family" o2 -O2
  run_regime "$family" ubsan -O2 -fsanitize=undefined \
    -fno-sanitize-recover=undefined
done

run_mutant archive m1 -DW34N61_MUTANT_MISSING_KSEG_PUBLICATION relocation.guest_publication
run_mutant archive m2 -DW34N61_MUTANT_SWAP_HEADER_OFFSETS relocation.header_offsets
run_mutant archive m3 -DW34N61_MUTANT_SKIP_COMPRESSED_FREE relocation.compressed_free
run_mutant lifecycle m4 -DW34N61_MUTANT_WRONG_TRANSITION_ARG setup.transition_args
run_mutant lifecycle m5 -DW34N61_MUTANT_WRONG_MODE_CONSTANT setup.state_values
run_mutant lifecycle m6 -DW34N61_MUTANT_WRONG_REGISTER_PAIR setup.registration_pairs
run_mutant lifecycle m7 -DW34N61_MUTANT_SKIP_SOUND_LINK setup.sound_link
run_mutant lifecycle m8 -DW34N61_MUTANT_SKIP_SEDS_FREE teardown.seds_free
run_mutant lifecycle m9 -DW34N61_MUTANT_WRONG_TEARDOWN_STATE teardown.state_values

rg -A3 'case 0x80077A64u:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_80077A64\(\)'
rg -A3 'case 0x80077CC0u:' "$ROOT/pc_port/src/world_map_main_loop_71034.c" |
  rg -q 'wm_80077CC0\(\)'
finish_seam="$(sed -n '/int wm_mode9_stage_second_wave_finish/,/}/p' \
  "$ROOT/pc_port/src/world_map_init.c")"
printf '%s\n' "$finish_seam" | rg -q 'wm_second_wave_poll\(\)'
printf '%s\n' "$finish_seam" | rg -q 'wm_80076954\(\)'
if rg -n '\(u32\)\(uintptr_t\)' \
  "$ROOT/pc_port/src/world_map_helper_76954.c" \
  "$ROOT/pc_port/src/world_map_mode9_lifecycle.c" >/dev/null; then
  echo "ERROR: mode-9 lifecycle contains host-pointer truncation" >&2
  exit 1
fi

echo "W34N61 MODE9 LIFECYCLE CERTIFICATE PASS: archive+lifecycle O0/O2/UBSan; M1-M9 detected; strict warnings clean"
