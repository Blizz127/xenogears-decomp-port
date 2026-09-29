#!/usr/bin/env bash
# Regression (D5): the port must find the user's retail files from any
# working directory, not just the repo root.  A run elsewhere with
# XENO_DATA_DIR set once left the main-exe static data (SLUS_006.64 .rodata /
# .sdata) zero-filled; battle's OT sentinel 0x8005698C then read 0 and the
# first battle stopped in DrawOTag.  This runs the port from a fresh temporary
# directory, configured only through the user-facing variables, warps into
# battle 1 from field map 1, and checks the data loaded and the battle runs.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
[ -x "$BIN" ] || { echo "FAIL: no port binary at $BIN"; exit 1; }
RUN="$(mktemp -d "${TMPDIR:-/tmp}/xeno-cwd.XXXXXX")"
trap 'rm -rf "$RUN"' EXIT
cd "$RUN"
export XENO_DATA_DIR="${XENO_DATA_DIR:-$ROOT/disc}"
export XENO_DISC="${XENO_DISC:-$ROOT/disc/disc1.bin}"
export XENO_BIOS="${XENO_BIOS:-$ROOT/disc/scph5500.bin}"
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" XENO_MODS_ENABLED=0
( sleep 25; echo 1 > bw.txt ) &
XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 XENO_BATTLE_WARP_FILE=bw.txt \
    timeout "${XENO_CWD_TEST_SECONDS:-90}" "$BIN" > run.log 2>&1
rc=$?
fail=0
grep -aq "main-exe static data loaded" run.log || { echo "FAIL: main-exe static data not loaded"; fail=1; }
grep -aq "zero-filled" run.log && { echo "FAIL: static data zero-filled"; fail=1; }
grep -aq "enter retail battle.bin" run.log || { echo "FAIL: battle not entered"; fail=1; }
grep -aq "invalid/cyclic DMA chain\|battle-mips\] stopped" run.log && { echo "FAIL: battle stopped:"; grep -a "battle-mips\] \(stopped\|invalid\)" run.log; fail=1; }
[ "$rc" = 124 ] || { echo "FAIL: run ended rc=$rc before the deadline"; fail=1; }
[ $fail = 0 ] && echo "PASS: retail data found from $RUN; battle runs"
exit $fail
