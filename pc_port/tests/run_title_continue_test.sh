#!/usr/bin/env bash
# Regression: selecting Continue on the title screen must open the retail
# Save/Load card screen and return to the title, not stop in GameHandleError.
#
# The r6.6rc title/New Game smoke hung about 1 run in 3-5 under host load:
# the pad schedule is clocked by real-time vblanks, so when the game thread
# lags, a scripted Circle lands one phase early/late and the title menu gets
# its confirm while the cursor is still on the default entry, Continue.  The
# port had no body for the card-screen controller func_801D9F98 (a generic
# stub returned 0), yet func_801C531C still ran the retail teardown
# (func_801E3088 -> func_801D9E3C -> func_801E5B3C), which freed 32 never-
# allocated slot pointers: HeapFree(NULL) -> MainLoop(0x83) ->
# GameHandleError spin, with the game thread never ticking again.
#
# This test forces that timing deterministically instead of waiting for load:
# the title cursor starts on Continue and a steady Circle pulse train (every
# CIRCLE_PERIOD vblanks) is phase-independent -- whichever pulse reaches the
# open title menu confirms Continue.  XENO_CARD_SCREEN_TEST=back
# (pc_port/src/psyq_compat.c) then retires the schedule once the card screen
# is up and backs out with one Cross release, clocked by menu reader ticks.
#
# Cases (both must pass; both hung before the fix):
#   cards   two fresh formatted virtual cards (XENO_MEMCARD_DIR in OUTDIR)
#   nocard  XENO_MEMCARD_SLOTS=0: retail shows the no-card message and the
#           screen closes by itself
#
# PASS per case: title loop entered; card screen entered (mode 1) and exited;
# the title loop resumed ("title confirm choice=1 keep=1"); no GameHandleError
# and no [stub] line; the game thread still ticking HOLD_SECONDS later.
# A hang (heartbeat lastTickAgoMs > STALL_MS) fails at once.
set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUTROOT="${XENO_TITLE_CONTINUE_OUTDIR:-$ROOT/scratchpad/title_continue/$STAMP}"
CASES="${XENO_TITLE_CONTINUE_CASES:-cards nocard}"
RUN_SECONDS="${XENO_TITLE_CONTINUE_SECONDS:-420}"
HOLD_SECONDS="${XENO_TITLE_CONTINUE_HOLD:-20}"
STALL_MS="${XENO_TITLE_CONTINUE_STALL_MS:-20000}"
CIRCLE_START="${XENO_TITLE_CONTINUE_CIRCLE_START:-3000}"
CIRCLE_PERIOD="${XENO_TITLE_CONTINUE_CIRCLE_PERIOD:-64}"
CIRCLE_END="${XENO_TITLE_CONTINUE_CIRCLE_END:-20000}"

cd "$ROOT"
mkdir -p "$OUTROOT"
if [ ! -x "$BINARY" ]; then
    echo "TITLE CONTINUE overall=NOT_RUN binary_missing=$BINARY" | tee "$OUTROOT/summary.txt"
    exit 2
fi
if [ ! -f disc/disc1.bin ]; then
    echo "TITLE CONTINUE overall=NOT_RUN disc_missing=disc/disc1.bin" | tee "$OUTROOT/summary.txt"
    exit 2
fi

# 300: Cross skips the opening STR (as in run_title_newgame_smoke.sh); then
# Circle 2-on pulses.  Every pulse either advances the attract/pad wait, opens
# the title (FE57) or, once the title loop is up, confirms its cursor.
schedule="0:0,300:0x40,320:0"
f=$CIRCLE_START
while [ "$f" -lt "$CIRCLE_END" ]; do
    schedule+=",${f}:0x20,$((f + 2)):0"
    f=$((f + CIRCLE_PERIOD))
done

overall=0
for case in $CASES; do
    OUT="$OUTROOT/$case"
    LOG="$OUT/run.log"
    mkdir -p "$OUT/memcards"
    extra=()
    case "$case" in
        cards) extra=(XENO_MEMCARD_DIR="$OUT/memcards") ;;
        nocard) extra=(XENO_MEMCARD_DIR="$OUT/memcards" XENO_MEMCARD_SLOTS=0) ;;
        *) echo "TITLE CONTINUE unknown case $case"; overall=1; continue ;;
    esac
    printf '%s\n' "$schedule" > "$OUT/schedule.txt"
    echo "TITLE CONTINUE case=$case binary=$BINARY out=$OUT"

    env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" \
        XENO_PAD_TEST_INPUT="$schedule" \
        XENO_CARD_SCREEN_TEST=back \
        "${extra[@]}" \
        timeout --signal=INT --kill-after=15 "${RUN_SECONDS}s" "$BINARY" \
        > "$LOG" 2>&1 &
    pid=$!

    verdict=""
    resumedAt=0
    start=$(date +%s)
    while kill -0 "$pid" 2>/dev/null; do
        sleep 2
        now=$(date +%s)
        if grep -a -q -F 'GameHandleError' "$LOG"; then verdict="FAIL game.handle.error"; break; fi
        age=$(grep -a -o 'lastTickAgoMs=[0-9]*' "$LOG" | tail -1 | cut -d= -f2)
        if [ -n "$age" ] && [ "$age" -gt "$STALL_MS" ]; then verdict="FAIL hang lastTickAgoMs=$age"; break; fi
        if [ "$resumedAt" -eq 0 ] &&
           grep -a -q -F '[xeno-port][menu] title confirm choice=1 keep=1' "$LOG"; then
            resumedAt=$now
        fi
        if [ "$resumedAt" -gt 0 ] && [ $((now - resumedAt)) -ge "$HOLD_SECONDS" ]; then
            verdict="observed"; break
        fi
        if [ $((now - start)) -ge "$RUN_SECONDS" ]; then verdict="FAIL deadline"; break; fi
    done
    kill -INT "$pid" 2>/dev/null; sleep 3
    pkill -KILL -P "$pid" 2>/dev/null; kill -KILL "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null
    [ -n "$verdict" ] || verdict="FAIL exited.early"

    fail=0
    check() { if [ "$2" -eq 1 ]; then echo "TITLE CONTINUE [$case] PASS $1"; else echo "TITLE CONTINUE [$case] FAIL $1"; fail=1; fi; }
    has() { grep -a -q -F -- "$1" "$LOG" && echo 1 || echo 0; }
    lastage=$(grep -a -o 'lastTickAgoMs=[0-9]*' "$LOG" | tail -1 | cut -d= -f2)
    check "run.observed.without.hang ($verdict)" "$([ "$verdict" = observed ] && echo 1 || echo 0)"
    check "title.loop.entered" "$(has '[xeno-port][menu] func_801C58EC title loop enter')"
    check "continue.opens.card.screen" "$(has '[xeno-port][menu] card screen enter mode=1')"
    check "card.screen.returns" "$(has '[xeno-port][menu] card screen exit result=0')"
    check "title.loop.resumes.after.continue" "$(has '[xeno-port][menu] title confirm choice=1 keep=1')"
    check "no.game.handle.error" "$([ "$(has 'GameHandleError')" = 0 ] && echo 1 || echo 0)"
    check "no.stub.reached" "$([ "$(has '[stub]')" = 0 ] && echo 1 || echo 0)"
    check "game.thread.still.ticking (lastTickAgoMs=${lastage:-none})" \
        "$([ -n "$lastage" ] && [ "$lastage" -lt 5000 ] && echo 1 || echo 0)"
    grep -a -n -F '[stub]' "$LOG" | head -5
    grep -a -A3 -F 'GameHandleError' "$LOG" | head -8
    if [ "$fail" -eq 0 ]; then echo "TITLE CONTINUE [$case] overall=PASS"; else echo "TITLE CONTINUE [$case] overall=FAIL"; overall=1; fi
done | tee "$OUTROOT/summary.txt"

overall=$(grep -c 'overall=FAIL' "$OUTROOT/summary.txt")
if [ "$overall" -eq 0 ]; then echo "TITLE CONTINUE overall=PASS"; exit 0; fi
echo "TITLE CONTINUE overall=FAIL"
exit 1
