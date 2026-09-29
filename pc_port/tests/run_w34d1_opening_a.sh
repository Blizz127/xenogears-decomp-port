#!/usr/bin/env bash
# N5 Lane A: Map 4 prologue -> Map 2 (Lahan opening), retail boot path.
#
# Uses the existing env seams only:
#   XENO_FIELD_MAP=4       selects the opening-narration map after the movie
#   XENO_FIELD_TEST_INPUT  deterministic 2-on/2-off Circle schedule
#   XENO_FIELD_CAPTURE_*   nonblack present-time capture evidence
#   XENO_NPC_EVENT_DUMP=1  dialog-open str progression from text_box.c
#
# Deliberately does NOT set XENO_FIELD_TEST (developer KernelMenu lane).
# Binary-absent and runtime-timeout runs stop with NOT_RUN gates, never a
# playable-Lahan claim.
set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HELPER="$ROOT/pc_port/tools/n5_opening_harness"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
RUN_SECONDS="${XENO_N5_RUN_SECONDS:-300}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUTDIR="${XENO_N5_OUTDIR:-$ROOT/scratchpad/n5_opening_harness/lane-a-$STAMP}"
LOGFILE="$OUTDIR/run.log"
CAPDIR="$OUTDIR/captures"

mkdir -p "$OUTDIR" "$CAPDIR"

if [ ! -x "$BINARY" ]; then
    {
        echo "N5 LANE A overall=NOT_RUN"
        echo "NOT_RUN_RC=2"
        echo "BUILD=NONE"
        echo "binary_missing=$BINARY"
        echo "A7_CONTROL=NOT_RUN"
        echo "note=no current xeno-port binary (N4 owns the build window)"
    } | tee "$OUTDIR/summary.txt"
    exit 2
fi

SCHEDULE="$("$HELPER/schedule.py")"
# Bash cannot combine the length operator with a pattern substitution in the
# same expansion ("${#SCHEDULE//[^,]}" is a bad substitution).  Split the
# generated CSV once and count its fields, keeping this diagnostic independent
# of external text-processing utilities.
IFS=, read -r -a SCHEDULE_STEPS <<< "$SCHEDULE"

echo "N5 LANE A binary=$BINARY outdir=$OUTDIR"
echo "schedule_steps=${#SCHEDULE_STEPS[@]}"

runner=()
if [ -z "${DISPLAY:-}" ] && [ -z "${SDL_VIDEODRIVER:-}" ]; then
    if command -v xvfb-run >/dev/null 2>&1; then
        runner=(xvfb-run -a)
        SDL_VIDEODRIVER=x11
    else
        {
            echo "N5 LANE A overall=NOT_RUN"
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
    XENO_FIELD_MAP=4 \
    XENO_FIELD_TEST_INPUT="$SCHEDULE" \
    XENO_FIELD_CAPTURE_DIR="$CAPDIR" \
    XENO_FIELD_CAPTURE_EVERY="${XENO_FIELD_CAPTURE_EVERY:-60}" \
    XENO_NPC_EVENT_DUMP=1 \
    SDL_VIDEODRIVER="$SDL_VIDEODRIVER" \
    timeout --signal=TERM --kill-after=10 "${RUN_SECONDS}s" "$BINARY" \
    >"$LOGFILE" 2>&1
rc=$?
set -e

echo "N5 LANE A runtime_rc=$rc"
case "$rc" in
    0|124|137|143) ;;
    *)
        {
            echo "N5 LANE A overall=FAIL"
            echo "runtime_rc=$rc"
            echo "A7_CONTROL=NOT_RUN"
        } | tee "$OUTDIR/summary.txt"
        tail -n 120 "$LOGFILE" >&2 || true
        exit 1
        ;;
esac

"$HELPER/analyze_log.py" "$LOGFILE" --lane A --capture-dir "$CAPDIR" \
    | tee "$OUTDIR/summary.txt"
rc=${PIPESTATUS[0]}
exit "$rc"
