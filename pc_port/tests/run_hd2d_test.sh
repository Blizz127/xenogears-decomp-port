#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT
cc -std=c11 -Wall -Wextra -Werror -Ipc_port/include -c pc_port/src/hd2d/hd2d_policy.c -o "$build_dir/policy.o"
cc -O2 -std=c11 -Wall -Wextra -Werror -Ipc_port/include -c pc_port/src/hd2d/hd2d_gl.c -o "$build_dir/gl.o"
c++ -std=c++11 -Wall -Wextra -Werror -Ipc_port/include $(pkg-config --cflags sdl2) \
    pc_port/tests/hd2d/hd2d_test.cpp "$build_dir/policy.o" "$build_dir/gl.o" \
    $(pkg-config --libs sdl2 gl) -o "$build_dir/test"
SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" LIBGL_ALWAYS_SOFTWARE="${LIBGL_ALWAYS_SOFTWARE:-1}" "$build_dir/test"
