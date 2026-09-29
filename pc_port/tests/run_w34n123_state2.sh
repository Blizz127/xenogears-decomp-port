#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=$(mktemp -d /tmp/w34n123-state2.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT

common=(
  -std=gnu17 -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
  -I"$repo_dir/pc_port/include_shim" -I"$repo_dir/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include/psx"
  -I"$repo_dir/pc_port/src"
  "$repo_dir/pc_port/tests/w34n123_state2_prod_test.c"
  "$repo_dir/pc_port/src/world_map_state2_8eb64.c"
  "$repo_dir/pc_port/src/world_map_vehicle_tail_90620.c"
)
warnings=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)

run_regime() {
  local name=$1
  shift
  gcc "${common[@]}" "${warnings[@]}" "$@" -o "$build_dir/$name"
  "$build_dir/$name" > "$build_dir/$name.log"
  grep -Fq 'W34N123 STATE2 CERTIFICATE PASS' "$build_dir/$name.log"
  printf 'W34N123 %s PASS\n' "$name"
}

run_regime O0 -O0
run_regime O2 -O2
run_regime UBSan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant() {
  local name=$1
  local define=$2
  local assertion=$3
  gcc "${common[@]}" "${warnings[@]}" -O2 "$define" -o "$build_dir/$name"
  if "$build_dir/$name" > "$build_dir/$name.log" 2>&1; then
    printf 'W34N123 %s SURVIVED\n' "$name" >&2
    return 1
  fi
  grep -Fq "FAIL [state2]: $assertion" "$build_dir/$name.log"
  printf 'W34N123 %s DETECTED by %s\n' "$name" "$assertion"
}

run_mutant M1 -DW34N123_MUTANT_M1_SKIP_EXIT_CODE_CLEAR \
  'terminal control clears frame and exit globals'
run_mutant M2 -DW34N123_MUTANT_M2_WRONG_TRANSITION_STATE \
  'successful transition selects state sixteen'
run_mutant M3 -DW34N123_MUTANT_M3_WRONG_WALKABILITY_MODE \
  'walkability lookup uses retail mode and signed region'
run_mutant M4 -DW34N123_MUTANT_M4_WRONG_ANGLE_OUTPUT \
  'angle search uses retail fixed output address'
run_mutant M5 -DW34N123_MUTANT_M5_WRONG_MOVEMENT_MODE \
  'movement helper receives retail ABI'
run_mutant M6 -DW34N123_MUTANT_M6_COPY_ANY_NONZERO_RESULT \
  'only exact result one may copy movement'
run_mutant M7 -DW34N123_MUTANT_M7_WRONG_TRIGGER_LIST \
  'trigger query uses retail list two'
run_mutant M8 -DW34N123_MUTANT_M8_SKIP_SECOND_MATRIX \
  'both retail matrices are built'
run_mutant M9 -DW34N123_MUTANT_M9_WRONG_PRESENCE_RECORD \
  'region two updates record 63 then releases 60'
run_mutant M10 -DW34N123_MUTANT_M10_WRONG_PUBLISH_STATES \
  'state sixteen publishes pose ring'

state_sha=$(dd if="$repo_dir/disc/world_map.bin" bs=1 \
  skip=$((0x8008eb64 - 0x8006faf0)) count=$((0x314)) status=none |
  sha256sum | awk '{print $1}')
test "$state_sha" = "$(dd if="$repo_dir/disc/world_map.bin" bs=1 \
  skip=$((0x8008eb64 - 0x8006faf0)) count=$((0x314)) status=none |
  sha256sum | awk '{print $1}')"
printf 'W34N123 RETAIL STATE SLICE SHA256 %s\n' "$state_sha"
printf 'W34N123 CERTIFICATE PASS O0/O2/UBSan; strict warnings; M1-M10 DETECTED\n'
