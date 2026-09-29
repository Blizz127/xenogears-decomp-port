#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PSYX="$ROOT/pc_port/extern/PsyCross"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xeno-movie-xa.XXXXXX")"
trap 'rm -rf "$BUILD_DIR"' EXIT

read -r -a SDL_CFLAGS <<<"$(pkg-config --cflags sdl2)"
read -r -a SDL_LIBS <<<"$(pkg-config --libs sdl2)"
read -r -a AL_LIBS <<<"$(pkg-config --libs openal)"

g++ -std=c++11 -ffunction-sections -fdata-sections -DXENO_PC_PORT \
    -I"$PSYX/include" -I"$PSYX/src" "${SDL_CFLAGS[@]}" \
    -c "$PSYX/src/audio/PsyX_SPUAL.cpp" -o "$BUILD_DIR/spual.o"
g++ -std=c++11 -ffunction-sections -fdata-sections -DXENO_PC_PORT \
    -I"$PSYX/include" -I"$PSYX/src" "${SDL_CFLAGS[@]}" \
    -c "$PSYX/src/psx/LIBCD.C" -o "$BUILD_DIR/libcd.o"
g++ -std=c++11 -ffunction-sections -fdata-sections -DXENO_PC_PORT \
    -I"$PSYX/include" -I"$PSYX/src" "${SDL_CFLAGS[@]}" \
    -c "$ROOT/pc_port/tests/movie_xa_decode_test.cpp" -o "$BUILD_DIR/test.o"
g++ -Wl,--gc-sections "$BUILD_DIR/test.o" "$BUILD_DIR/spual.o" \
    "$BUILD_DIR/libcd.o" \
    "${SDL_LIBS[@]}" "${AL_LIBS[@]}" -o "$BUILD_DIR/test"
"$BUILD_DIR/test"

if [ ! -f "$ROOT/disc/disc1.bin" ]; then
    echo "movie XA Disc 1 golden: NOT_RUN (missing disc/disc1.bin)" >&2
    exit 1
fi
python3 "$ROOT/pc_port/tests/xa_retail_oracle.py" \
    "$ROOT/disc/disc1.bin" "$BUILD_DIR/oracle.pcm"
"$BUILD_DIR/test" "$ROOT/disc/disc1.bin" "$BUILD_DIR/production.pcm"
cmp "$BUILD_DIR/oracle.pcm" "$BUILD_DIR/production.pcm"
echo "movie XA Disc 1 two-sector decode/zigzag continuity: PASS"
