#!/usr/bin/env bash
# Authenticate the player-supplied North American world overlay.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/../../.."
world_image=${WMD_IMAGE:-disc/world_map.bin}
digest=$(sha256sum < "$world_image")
expected=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
if [[ ${digest%% *} != "$expected" ]]; then
    echo "World overlay identity mismatch" >&2
    exit 1
fi

run_world_guest_fixture() {
    local source=$1 build
    build=$(mktemp -d "${TMPDIR:-/tmp}/xeno-world-guest.XXXXXXXX")
    (
        ulimit -c 0
        trap 'rm -rf "$build"' EXIT
        "${CC:-cc}" -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src \
            "$source" pc_port/src/battle_mips_adapter.c -o "$build/test"
        env -u XENO_DISABLE_CPU_INTERPRETER timeout 90 "$build/test" "$world_image"
    )
}
