#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
task_hd2d_tmp=$(mktemp -d)
trap 'rm -rf "$task_hd2d_tmp"' EXIT
# Conventional whitespace-separated compiler/flag overrides; no eval.
read -r -a task_hd2d_cc <<< "${CC:-cc}"
read -r -a task_hd2d_cppflags <<< "${CPPFLAGS:-}"
read -r -a task_hd2d_modes <<< "${MODES:-O0 O2 UBSan}"
for api in present absent; do
    flags=()
    if [[ "$api" == absent ]]; then flags=(-DHD2D_TEST_NO_VALUE_API); fi
    for mode in "${task_hd2d_modes[@]}"; do
        checks=(-"$mode")
        if [[ "$mode" == UBSan ]]; then checks=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
        "${task_hd2d_cc[@]}" -std=c11 -Wall -Wextra -Werror "${flags[@]}" "${checks[@]}" \
            "${task_hd2d_cppflags[@]}" -UNDEBUG -Ipc_port/include_shim -Ipc_port/include -Ipc_port/src \
            pc_port/src/hd2d/hd2d_bridge.c pc_port/src/hd2d/hd2d_policy.c \
            pc_port/src/hd2d/hd2d_scene_buffers_adapter.c \
            pc_port/tests/hd2d/hd2d_bridge_test.c -o "$task_hd2d_tmp/test"
        env -u XENO_HD2D_OCTOPATH -u XENO_HD2D_OPENING_MAPS "$task_hd2d_tmp/test"
    done
done
