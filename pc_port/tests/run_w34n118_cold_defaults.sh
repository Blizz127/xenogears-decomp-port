#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=$(mktemp -d /tmp/w34n118-cold-defaults.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT

common=(
  -std=gnu17 -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
  -I"$repo_dir/pc_port/include_shim" -I"$repo_dir/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include/psx"
  -I"$repo_dir/pc_port/src"
  "$repo_dir/pc_port/tests/w34n118_cold_defaults_prod_test.c"
  "$repo_dir/pc_port/src/world_map_cold_defaults.c"
)
warnings=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)

run_regime() {
  local name=$1
  shift
  gcc "${common[@]}" "${warnings[@]}" "$@" -o "$build_dir/$name"
  "$build_dir/$name" > "$build_dir/$name.log"
  grep -Fq 'W34N118 COLD DEFAULTS CERTIFICATE PASS' "$build_dir/$name.log"
  printf 'W34N118 %s PASS\n' "$name"
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
    printf 'W34N118 %s SURVIVED\n' "$name" >&2
    return 1
  fi
  grep -Fq "FAIL [cold]: $assertion" "$build_dir/$name.log"
  printf 'W34N118 %s DETECTED by %s\n' "$name" "$assertion"
}

run_mutant M1 -DW34N118_MUTANT_KEEP_ZERO_ENTRANCE \
  'entrance coerced to one'
run_mutant M2 -DW34N118_MUTANT_BAD_CHANNEL2 \
  'channel two default is five'
run_mutant M3 -DW34N118_MUTANT_SKIP_HOST_MIRROR \
  'host/guest alias mirrors exact'
run_mutant M4 -DW34N118_MUTANT_WRONG_TABLE_INDEX \
  'lookup pair uses incoming EE7A'
run_mutant M5 -DW34N118_MUTANT_DROP_D160 \
  'D160 fixed upper bound'

printf 'W34N118 CERTIFICATE PASS O0/O2/UBSan; strict warnings; M1-M5 DETECTED\n'
