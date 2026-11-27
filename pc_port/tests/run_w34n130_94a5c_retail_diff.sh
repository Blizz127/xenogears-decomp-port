#!/usr/bin/env bash
# Exercise the guest collision router with deterministic boundary service doubles.
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
image=${1:-"$root/disc/world_map.bin"}
if [[ $# -gt 1 || ! -f "$image" ]]; then
    echo "Supply your US Disc 1 world_map.bin as the optional input path." >&2
    exit 2
fi
hash=$(sha256sum < "$image")
if [[ ${hash%% *} != 4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70 ]]; then
    echo "Unrecognized US Disc 1 world_map.bin." >&2
    exit 2
fi
out=$(mktemp -d "${TMPDIR:-/tmp}/w34n130.XXXXXXXX")
trap 'rm -rf "$out"' EXIT
for mode in normal ubsan; do
    flags=(-O2)
    if [[ $mode == ubsan ]]; then flags+=(-fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-cc}" -std=gnu17 -Wall -Wextra -Werror "${flags[@]}" -I"$root/pc_port/src" \
        "$root/pc_port/tests/w34n130_94a5c_retail_diff_test.c" \
        "$root/pc_port/src/battle_mips_adapter.c" -o "$out/router"
    XENO_DISABLE_CPU_INTERPRETER=0 "$out/router" "$image"
done
