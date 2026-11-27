#!/usr/bin/env bash
# Check the owned guest fragment at its fog service boundary.
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
world=${1:-"$root/disc/world_map.bin"}
if ! digest=$(sha256sum < "$world") ||
   [[ ${digest%% *} != 4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70 ]]; then
    echo "Supply the world_map.bin extracted from your US Disc 1." >&2
    exit 2
fi
work=$(mktemp -d "${TMPDIR:-/tmp}/world-fog.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
"${CC:-cc}" -std=gnu17 -O2 -Wall -Wextra -Werror -I"$root/pc_port/src" \
    "$root/pc_port/tests/w34n129_fog_near_far_test.c" "$root/pc_port/src/battle_mips_adapter.c" \
    -o "$work/world-fog"
for strict in 0 1; do
    XENO_DISABLE_CPU_INTERPRETER="$strict" "$work/world-fog" "$world"
done
