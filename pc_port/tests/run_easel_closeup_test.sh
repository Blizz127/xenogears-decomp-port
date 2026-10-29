#!/usr/bin/env bash
# Runtime check (needs the disc under disc/ and a field-14 quick checkpoint in
# XENO_TEST_FIELD14_CHECKPOINT): the easel close-up in Fei's room leaves on
# confirm only, as the retail field script does.  See easel_closeup_test.py.
# HEADLESS: offscreen GL, never a window.
set -u
unset DISPLAY WAYLAND_DISPLAY XAUTHORITY
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
OUT="${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/easel_closeup/$(date +%Y%m%d-%H%M%S)"
CKPT="${XENO_TEST_FIELD14_CHECKPOINT:-}"
if [ ! -x "$BINARY" ] || [ ! -f "$ROOT/disc/disc1.bin" ]; then
    echo "EASEL CLOSEUP overall=NOT_RUN (binary or disc missing)"
    exit 2
fi
if [ -z "$CKPT" ] || [ ! -f "$CKPT" ]; then
    echo "EASEL CLOSEUP overall=NOT_RUN (set XENO_TEST_FIELD14_CHECKPOINT)"
    exit 2
fi
exec timeout --kill-after=15 900 python3 "$ROOT/pc_port/tests/easel_closeup_test.py" \
    "$BINARY" "$ROOT" "$OUT" "$CKPT"
