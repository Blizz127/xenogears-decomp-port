#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PSYX="$ROOT/pc_port/extern/PsyCross"
BUILD_DIR="$(mktemp -d)"
trap 'rm -rf "$BUILD_DIR"' EXIT

cxxflags=(
    -std=c++11 -ffunction-sections -fdata-sections
    -I"$PSYX/include" -I"$PSYX/src"
)
read -r -a sdl_cflags <<<"$(pkg-config --cflags sdl2)"
read -r -a sdl_libs <<<"$(pkg-config --libs sdl2)"

g++ "${cxxflags[@]}" "${sdl_cflags[@]}" \
    -c "$PSYX/src/pad/PsyX_pad.cpp" -o "$BUILD_DIR/PsyX_pad.o"
g++ "${cxxflags[@]}" "${sdl_cflags[@]}" \
    -c "$ROOT/pc_port/tests/psycross_keyboard_tap_test.cpp" \
    -o "$BUILD_DIR/test.o"
g++ -Wl,--gc-sections "$BUILD_DIR/test.o" "$BUILD_DIR/PsyX_pad.o" \
    "${sdl_libs[@]}" -o "$BUILD_DIR/test"

"$BUILD_DIR/test"
