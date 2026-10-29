#!/usr/bin/env bash
# Runtime regression (needs the disc under disc/): no VRAM garbage frame is
# presented when a menu closes (title menu -> New Game -> FE60 STR).  See
# menu_close_present_test.py.  HEADLESS: offscreen GL, never a window.
set -u
unset DISPLAY WAYLAND_DISPLAY XAUTHORITY
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
OUT="${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/menu_close_present/$(date +%Y%m%d-%H%M%S)"
if [ ! -x "$BINARY" ] || [ ! -f "$ROOT/disc/disc1.bin" ]; then
    echo "MENU CLOSE PRESENT overall=NOT_RUN (binary or disc missing)"
    exit 2
fi
exec timeout --kill-after=15 900 python3 "$ROOT/pc_port/tests/menu_close_present_test.py" \
    "$BINARY" "$ROOT" "$OUT"
