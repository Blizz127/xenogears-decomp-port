#!/usr/bin/env bash
# Input trace never throttles button events (pc_port/tests/input_trace_test.cpp).
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/input-trace.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
g++ -std=gnu++17 -O1 -g -Wall $(pkg-config --cflags sdl2) pc_port/tests/input_trace_test.cpp \
    $(pkg-config --libs sdl2) -o "$OUT/input_trace_test"
SDL_VIDEODRIVER=dummy timeout 60s "$OUT/input_trace_test"
