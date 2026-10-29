#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
task_menu_tmp=$(mktemp -d)
trap 'rm -rf "$task_menu_tmp"' EXIT
# Conventional whitespace-separated compiler/flag overrides; no eval.
read -r -a task_menu_cc <<< "${CC:-cc}"
read -r -a task_menu_cppflags <<< "${CPPFLAGS:-}"
read -r -a task_menu_modes <<< "${MODES:-O0 O2 UBSan}"
for mode in "${task_menu_modes[@]}"; do
    checks=(-"$mode")
    if [[ "$mode" == UBSan ]]; then checks=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${task_menu_cc[@]}" -std=c11 -Wall -Wextra -Werror "${checks[@]}" "${task_menu_cppflags[@]}" -UNDEBUG \
        -Ipc_port/include_shim -Ipc_port/include -Ipc_port/src \
        pc_port/src/port_dev_menu.c pc_port/src/hd2d/hd2d_bridge.c \
        pc_port/src/hd2d/hd2d_scene_buffers_adapter.c \
        pc_port/src/hd2d/hd2d_policy.c pc_port/tests/dev_menu_enhancement_test.c \
        -o "$task_menu_tmp/test"
    env -u XENO_DEV_MENU -u XENO_CHEATS -u XENO_HD2D_OCTOPATH \
        -u XENO_HD2D_OPENING_MAPS "$task_menu_tmp/test"
done
