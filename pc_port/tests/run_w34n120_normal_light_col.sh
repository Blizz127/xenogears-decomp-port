#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=$(mktemp -d /tmp/w34n120-normal-light-col.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT

common=(
  -std=gnu17 -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
  -I"$repo_dir/pc_port/include_shim" -I"$repo_dir/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include/psx"
  -I"$repo_dir/pc_port/src"
  "$repo_dir/pc_port/tests/w34n120_normal_light_col_prod_test.c"
  "$repo_dir/pc_port/src/psyq_compat_normal_light_col.c"
)
warnings=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)

run_regime() {
  local name=$1
  shift
  gcc "${common[@]}" "${warnings[@]}" "$@" -o "$build_dir/$name"
  "$build_dir/$name" > "$build_dir/$name.log"
  grep -Fq 'W34N120 NORMAL LIGHT COL CERTIFICATE PASS' "$build_dir/$name.log"
  printf 'W34N120 %s PASS\n' "$name"
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
    printf 'W34N120 %s SURVIVED\n' "$name" >&2
    return 1
  fi
  grep -Fq "FAIL [normal-light-col]: $assertion" "$build_dir/$name.log"
  printf 'W34N120 %s DETECTED by %s\n' "$name" "$assertion"
}

run_mutant M1 -DW34N120_MUTANT_WRONG_VXY_REGISTER \
  'normal word zero loads VXY register'
run_mutant M2 -DW34N120_MUTANT_DROP_VZ_LOAD \
  'normal word one loads VZ register'
run_mutant M3 -DW34N120_MUTANT_WRONG_RGBC_REGISTER \
  'input color loads RGBC register'
run_mutant M4 -DW34N120_MUTANT_WRONG_OPCODE \
  'retail NCCS opcode'
run_mutant M5 -DW34N120_MUTANT_WRONG_OUTPUT_REGISTER \
  'reads RGB2 register'
run_mutant M6 -DW34N120_MUTANT_CORRUPT_OUTPUT \
  'stores RGB2 output'

printf 'W34N120 CERTIFICATE PASS O0/O2/UBSan; strict warnings; M1-M6 DETECTED\n'
