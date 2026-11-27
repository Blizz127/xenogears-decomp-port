#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
world=${1:-disc/world_map.bin}
expected_world=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
read -r digest _ < <(sha256sum < "$world")
if [[ $digest != "$expected_world" ]]; then
    echo "Expected the US Disc 1 world_map.bin from your own disc." >&2
    exit 2
fi
out=$(mktemp -d "${TMPDIR:-/tmp}/xeno-world-probes.XXXXXXXX")
trap 'rm -rf "$out"' EXIT
for mode in O2 UBSan; do
    flags=(-O2)
    if [[ $mode == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-cc}" -std=gnu17 -Wall -Wextra -Werror "${flags[@]}" -Ipc_port/src \
        pc_port/tests/w34n131_private_probe_retail_diff_test.c pc_port/src/battle_mips_adapter.c -o "$out/probes"
    # Independent REMU reference for the same owned-code trace and scripted services.
    trace=$("$out/probes" "$world" | sha256sum)
    if [[ ${trace%% *} != d5a6715a2533f8f2591eab3f4536f6c5a84b66eda9b86158662eaf7438dd4c6f ]]; then
        echo "$mode collision probe trace disagrees with the reference." >&2
        exit 1
    fi
done
echo "Verified four guest collision probes against reference call, result, instruction count and scratch traces."
