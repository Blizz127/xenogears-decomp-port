#!/usr/bin/env bash
# N5 Lane B: normal boot -> movie -> Map 490 title preflight.
#
# Stops at the title loop marker.  The New Game confirm has no deterministic
# PC seam (func_801C7D78 overwrites g_Menu->input every frame; the prior GDB
# chain injected after the input reader), so the full 490 -> 4 -> 2 chain is
# reported NEW_GAME=NOT_RUN / FIELD_CHAIN=NOT_RUN rather than claimed.
#
# XENO_FIELD_TEST_INPUT is still armed so the Map 490 attract's OP31 Circle
# check can open the title deterministically; the title menu reader drains the
# pad queue, not those merged accumulators, so the schedule cannot confirm New
# Game and we leave the inject seam untouched.
set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HELPER="$ROOT/pc_port/tools/n5_opening_harness"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
RUN_SECONDS="${XENO_N5_BOOT_SECONDS:-400}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUTDIR="${XENO_N5_OUTDIR:-$ROOT/scratchpad/n5_opening_harness/lane-b-$STAMP}"
LOGFILE="$OUTDIR/run.log"
CAPDIR="$OUTDIR/captures"

mkdir -p "$OUTDIR" "$CAPDIR"

if [ ! -x "$BINARY" ]; then
    {
        echo "N5 LANE B overall=NOT_RUN"
        echo "NOT_RUN_RC=2"
        echo "BUILD=NONE"
        echo "binary_missing=$BINARY"
        echo "NEW_GAME=NOT_RUN"
        echo "FIELD_CHAIN=NOT_RUN"
        echo "A7_CONTROL=NOT_RUN"
        echo "note=no current xeno-port binary (N4 owns the build window)"
    } | tee "$OUTDIR/summary.txt"
    exit 2
fi

SCHEDULE="$("$HELPER/schedule.py")"

echo "N5 LANE B binary=$BINARY outdir=$OUTDIR"

runner=()
if [ -z "${DISPLAY:-}" ] && [ -z "${SDL_VIDEODRIVER:-}" ]; then
    if command -v xvfb-run >/dev/null 2>&1; then
        runner=(xvfb-run -a)
        SDL_VIDEODRIVER=x11
    else
        {
            echo "N5 LANE B overall=NOT_RUN"
            echo "NOT_RUN_RC=2"
            echo "BUILD=PRESENT"
            echo "DISPLAY=NONE"
            echo "note=no DISPLAY and xvfb-run unavailable"
        } | tee "$OUTDIR/summary.txt"
        exit 2
    fi
else
    SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"
fi

set +e
"${runner[@]}" env \
    XENO_FIELD_TEST_INPUT="$SCHEDULE" \
    XENO_FIELD_CAPTURE_DIR="$CAPDIR" \
    XENO_FIELD_CAPTURE_EVERY="${XENO_FIELD_CAPTURE_EVERY:-60}" \
    XENO_NPC_EVENT_DUMP=1 \
    SDL_VIDEODRIVER="$SDL_VIDEODRIVER" \
    timeout --signal=TERM --kill-after=10 "${RUN_SECONDS}s" "$BINARY" \
    >"$LOGFILE" 2>&1
rc=$?
set -e

echo "N5 LANE B runtime_rc=$rc"
case "$rc" in
    0|124|137|143)
        ;;  # timeout / terminated: expected for a boot-to-title preflight
    *)
        {
            echo "N5 LANE B overall=FAIL"
            echo "runtime_rc=$rc"
            echo "NEW_GAME=NOT_RUN"
            echo "FIELD_CHAIN=NOT_RUN"
            echo "A7_CONTROL=NOT_RUN"
        } | tee "$OUTDIR/summary.txt"
        tail -n 120 "$LOGFILE" >&2 || true
        exit 1
        ;;
esac

"$HELPER/analyze_log.py" "$LOGFILE" --lane B --capture-dir "$CAPDIR" \
    | tee "$OUTDIR/summary.txt"
rc=${PIPESTATUS[0]}
exit "$rc"
