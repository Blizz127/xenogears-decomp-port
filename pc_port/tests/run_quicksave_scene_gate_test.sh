#!/usr/bin/env bash
# Runtime regression (needs the disc under disc/, ~15 min headless): quick
# saves are refused during the field-14 painting scene with
# "CAN'T QUICKSAVE DURING A SCENE", the one taken after the scene matches the
# live scenario flag, and loading it replays nothing.  See
# quicksave_scene_gate_test.py.  Extra args are passed through
# (--load-only CKPT checks only the load phase).  HEADLESS: offscreen GL.
set -u
unset DISPLAY WAYLAND_DISPLAY XAUTHORITY
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
OUT="${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/quicksave_scene_gate/$(date +%Y%m%d-%H%M%S)"
if [ ! -x "$BINARY" ] || [ ! -f "$ROOT/disc/disc1.bin" ]; then
    echo "QUICKSAVE SCENE overall=NOT_RUN (binary or disc missing)"
    exit 2
fi
exec timeout --kill-after=15 2700 python3 "$ROOT/pc_port/tests/quicksave_scene_gate_test.py" \
    "$BINARY" "$ROOT" "$OUT" "$@"
