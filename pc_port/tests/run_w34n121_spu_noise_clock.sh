#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/w34n121-spu-noise-clock.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT

provider="$repo_dir/pc_port/src/psyq_compat_spu_noise_clock.c"
common=(
  -std=gnu17 -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
  -I"$repo_dir/pc_port/include_shim" -I"$repo_dir/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include/psx"
  -I"$repo_dir/pc_port/src"
  "$repo_dir/pc_port/tests/w34n121_spu_noise_clock_prod_test.c"
)
warnings=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)

run_regime() {
  local name=$1
  shift
  gcc "${common[@]}" "${warnings[@]}" "$@" "$provider" -o "$build_dir/$name"
  "$build_dir/$name" > "$build_dir/$name.log"
  grep -Fq 'W34N121 SPU NOISE CLOCK CERTIFICATE PASS' "$build_dir/$name.log"
  printf 'W34N121 %s PASS\n' "$name"
}

run_regime O0 -O0
run_regime O2 -O2
run_regime UBSan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant() {
  local name=$1
  local original=$2
  local replacement=$3
  local assertion=$4
  local mutant="$build_dir/$name.c"
  python3 "$repo_dir/pc_port/tests/lib/write_source_mutant.py" \
    "$provider" "$mutant" "$original" "$replacement"
  gcc "${common[@]}" "${warnings[@]}" -O2 "$mutant" -o "$build_dir/$name"
  local status=0
  "$build_dir/$name" > "$build_dir/$name.log" 2>&1 || status=$?
  if [[ $status != 1 ]]; then
    printf 'Mutant %s returned %s (expected assertion failure exit 1)\n' "$name" "$status" >&2
    return 1
  fi
  grep -Fq "FAIL [spu-noise-clock]: $assertion" "$build_dir/$name.log"
  printf 'W34N121 %s DETECTED by %s\n' "$name" "$assertion"
}

run_mutant M1 \
  'noise_clock = 0;' \
  'noise_clock = 1;' \
  'negative input clamps to zero'
run_mutant M2 \
  'noise_clock = 63;' \
  'noise_clock = 62;' \
  'above-range input clamps to 63'
run_mutant M3 \
  '0xc0ffu' \
  '0x00ffu' \
  'non-clock SPUCNT bits preserved'
run_mutant M4 \
  '<< 8' \
  '<< 7' \
  'in-range value occupies SPUCNT bits 13 through 8'
run_mutant M5 \
  '0x1aa' \
  '0x1a8' \
  'writes retail SPUCNT offset'
run_mutant M6 \
  'return noise_clock;' \
  'return (long)saved;' \
  'above-range input clamps to 63'

printf 'W34N121 CERTIFICATE PASS O0/O2/UBSan; strict warnings; M1-M6 DETECTED\n'
