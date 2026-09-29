#!/usr/bin/env bash
# A second world-map session in one visit must set up like retail instead of
# exiting (pc_port/tools/world_harness/w34_second_session.gdb).
set -uo pipefail
cd "$(dirname "$0")/../.."
log=${WM2_LOG:-$(mktemp "${TMPDIR:-/tmp}/wm2.XXXXXX.log")}
XENO_WORLD_TEST_INPUT='0:0x2000,30:0,40:0x20,41:0' SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" \
  timeout -s INT "${WM2_TIMEOUT:-420}" gdb -batch -x pc_port/tools/world_harness/w34_second_session.gdb \
  --args pc_port/build_native/xeno-port > "$log" 2>&1
grep -a "^WM2 " "$log"
if grep -aq "^WM2 second-session setup rc=0$" "$log" && ! grep -aq "exited with code" "$log"; then
    echo "WORLD MAP SECOND SESSION PASS"; exit 0
fi
echo "WORLD MAP SECOND SESSION FAIL (log $log)"; tail -5 "$log"; exit 1
