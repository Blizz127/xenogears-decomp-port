#!/usr/bin/env bash
# Live test of streamed-file asset replacement (xg_plat/mods.h, archive_port.c).
#
# Field map 1 on the XENO_FIELD_TEST route loads its sound bank through the
# field stream loader (func_80085560, CdlModeStream).  The test dumps every
# original the run loads (from the user's own disc), installs them as an
# identity mod, and runs the same route again.  It passes when the streamed
# file is served from the mod (no CD read) and the run behaves exactly like
# the unmodded one (same deadline exit, same field-diag trace length).
#
# The dump is game data: it lives in a temporary directory outside the repo
# and is deleted on exit.  Needs the disc image and BIOS like the other
# retail runners.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
SECONDS_RUN="${XENO_STREAM_TEST_SECONDS:-45}"
TMP="$(mktemp -d "${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/stream_live.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

[ -x "$BIN" ] || { echo "FAIL: no port binary at $BIN"; exit 1; }
export XENO_FIELD_TEST=1 XENO_FIELD_MAP=1 XENO_KERNEL_SEL=0
export XENO_DATA_DIR="${XENO_DATA_DIR:-$ROOT/disc}"
export XENO_DISC="${XENO_DISC:-$ROOT/disc/disc1.bin}"
export XENO_BIOS="${XENO_BIOS:-$ROOT/disc/scph5500.bin}"
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}"

cd "$TMP"
mkdir dump
set +e
XENO_MODS_DUMP_DIR=dump timeout "$SECONDS_RUN" "$BIN" > dump.log 2>&1; rc1=$?
set -e
n=$(ls dump | wc -l)
[ "$n" -gt 0 ] || { echo "FAIL: dump run saved no originals (rc=$rc1)"; exit 1; }

mkdir -p mods/identity/assets
printf 'name = identity\nabi = 1\n' > mods/identity/mod.txt
mv dump/*.bin mods/identity/assets/
set +e
timeout "$SECONDS_RUN" "$BIN" > mod.log 2>&1; rc2=$?
set -e

fail=0
streams=$(grep -c 'streamed file at sector .* served from a replacement' mod.log || true)
d1=$(grep -c 'field-diag' dump.log || true)
d2=$(grep -c 'field-diag' mod.log || true)
echo "dumped=$n streams_replaced=$streams rc(dump)=$rc1 rc(mod)=$rc2 field-diag=$d1/$d2"
[ "$streams" -ge 1 ] || { echo "FAIL: no streamed file was served from the mod"; fail=1; }
[ "$rc1" = 124 ] && [ "$rc2" = 124 ] || { echo "FAIL: a run did not reach the deadline"; fail=1; }
[ "$d1" = "$d2" ] || { echo "FAIL: modded run diverged from the unmodded run"; fail=1; }
[ $fail = 0 ] && echo "PASS: streamed replacement served live on field map 1"
exit $fail
