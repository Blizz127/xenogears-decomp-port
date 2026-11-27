#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../../.." && pwd)
case "$1" in
    battle_popup_frame_data_test|battle_popup_frame_decode_test|battle_menu_data_test|battle_menu_alloc_continuation_test|battle_popup_colour_prefix_test|battle_popup_owner_next_test|battle_popup_owner_prefix_test|battle_popup_owner_loop_test|battle_popup_task_suffix_test) fixture=$1 ;;
    *) exit 2 ;;
esac
media=$(cd "${XENO_DATA_DIR:-$root/disc}" && pwd)
slus="$media/SLUS_006.64"
battle="$media/battle.bin"
[[ $(sha256sum "$slus" | awk '{print $1}') == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]]
[[ $(sha256sum "$battle" | awk '{print $1}') == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]]
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
sources=("$root/pc_port/src/battle_mips_adapter.c")
if [[ "$fixture" == battle_popup_frame_* ]]; then
    sources+=("$root/pc_port/src/data_battle_popup_frame.c")
fi
"${CC:-cc}" -std=gnu17 -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
    -I"$root/pc_port/src" "$root/pc_port/tests/$fixture.c" "${sources[@]}" \
    -Wl,--gc-sections -o "$tmp/$fixture"
env -u XENO_DISABLE_CPU_INTERPRETER XENO_DATA_DIR="$media" "$tmp/$fixture" "$slus" "$battle"
