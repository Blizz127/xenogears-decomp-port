#!/usr/bin/env bash
set -euo pipefail
if [[ $# != 2 ]]; then
    echo "Usage: $0 <production animation_scripts.c.o> <owner SLUS_006.64>" >&2
    exit 2
fi
object=$(realpath "$1")
image=$(realpath "$2")
actual=$(sha256sum "$image")
if [[ ${actual%% *} != dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]]; then
    echo "Unexpected owner executable" >&2
    exit 2
fi
root=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=${TMPDIR:-"$root/pc_port/build_native"}
mkdir -p "$build_dir"
tmp=$(mktemp -d "$build_dir/xeno-opening-bb.XXXXXXXX")
trap 'rm -rf "$tmp"' EXIT
"${CC:-cc}" -std=c17 -O2 -Wall -Wextra -Werror -fno-pie -no-pie \
    -ffunction-sections -fdata-sections -Wl,--gc-sections -I"$root/pc_port/src" \
    "$root/pc_port/tests/sprite_dispatch_bb_opening_test.c" \
    "$root/pc_port/src/battle_mips_adapter.c" "$object" -o "$tmp/test"
"$tmp/test" "$image"
