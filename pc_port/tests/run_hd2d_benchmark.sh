#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT
cc -O2 -std=c11 -Wall -Wextra -Werror -Ipc_port/include -c pc_port/src/hd2d/hd2d_policy.c -o "$build_dir/policy.o"
cc -O2 -std=c11 -Wall -Wextra -Werror -Ipc_port/include -c pc_port/src/hd2d/hd2d_gl.c -o "$build_dir/gl.o"
c++ -O2 -std=c++11 -Wall -Wextra -Werror -Ipc_port/include $(pkg-config --cflags sdl2) \
    pc_port/tests/hd2d/hd2d_benchmark.cpp "$build_dir/policy.o" "$build_dir/gl.o" \
    $(pkg-config --libs sdl2 gl) -o "$build_dir/benchmark"
# Inherit the runner's display/GPU selection. Do not force software rendering.
"$build_dir/benchmark" "$@"
