#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=$(mktemp -d /tmp/w34n127-state01.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT

common=(
  -std=gnu17 -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
  -I"$repo_dir/pc_port/include_shim" -I"$repo_dir/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include"
  -I"$repo_dir/pc_port/extern/PsyCross/include/psx"
  -I"$repo_dir/pc_port/src"
  "$repo_dir/pc_port/tests/w34n127_vehicle_state01_prod_test.c"
  "$repo_dir/pc_port/src/world_map_callback_8e76c.c"
  "$repo_dir/pc_port/src/world_map_state01_8eb30.c"
  "$repo_dir/pc_port/src/world_map_vehicle_tail_90620.c"
)
warnings=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)

run_regime() {
  local name=$1
  shift
  gcc "${common[@]}" "${warnings[@]}" "$@" -o "$build_dir/$name"
  "$build_dir/$name" > "$build_dir/$name.log"
  grep -Fq 'W34N127 VEHICLE STATE01 CERTIFICATE PASS' "$build_dir/$name.log"
  printf 'W34N127 %s PASS\n' "$name"
}

run_regime O0 -O0
run_regime O2 -O2
run_regime UBSan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

state_sha=$(dd if="$repo_dir/disc/world_map.bin" bs=1 \
  skip=$((0x8008eb30 - 0x8006faf0)) count=$((0x34)) status=none |
  sha256sum | awk '{print $1}')
tail_sha=$(dd if="$repo_dir/disc/world_map.bin" bs=1 \
  skip=$((0x80090620 - 0x8006faf0)) count=$((0x94)) status=none |
  sha256sum | awk '{print $1}')
test "$state_sha" = 0f0e3b8819a5c572a771ca2ae6c31117586904d6af7261163e0ef20699627db0
test "$tail_sha" = d1e69ec3a0803d27cac240edc37e22659741c30b8ccfb2d7c38606d8f1adde5c
printf 'W34N127 RETAIL STATE01 SHA256 %s\n' "$state_sha"
printf 'W34N127 RETAIL SHARED TAIL SHA256 %s\n' "$tail_sha"
printf 'W34N127 CERTIFICATE PASS O0/O2/UBSan; strict warnings\n'
