#!/usr/bin/env bash
# Retail boot -> title (Map 490) -> New Game -> prologue (Map 4) -> Lahan
# (Map 2) runtime smoke, driven entirely at the raw pad layer.
#
# Uses the existing env seams only:
#   XENO_PAD_TEST_INPUT    "frame:value" schedule at the BIOS pad buffer
#                          (pc_port/src/psyq_compat.c); the clock is Vsync-shim
#                          calls, NOT field frames.
#   XENO_VM_TRACE=0        distinct-ip opcode trace for actor 0 (the title
#                          map's only actor) -- src/field/scripts/virtual_machine.c
#   XENO_FIELD_CAPTURE_*   frame captures (BMP payload despite the .png name)
#
# Why these buttons: the title map only answers CIRCLE (0x20).  Its FE60 body
# func_800A7C58 polls D_800C3900 & 0x20 to interrupt the attract STR, the
# script's pad wait then opens the title on Circle (FE57), the title menu's
# reader func_801C7D78 confirms on the Circle RELEASE edge, and UP (0x1000)
# moves the default cursor (Continue, choice 1) to New Game (choice 2).
# Cross / Start / d-pad do nothing on that map, which is retail behaviour.
#
# Deliberately does NOT set XENO_FIELD_TEST (developer KernelMenu lane).
#
# XENO_TITLE_SMOKE_CONTROL=cross swaps every Circle for Cross: the chain must
# then NOT reach the title menu, and the script exits 0 only if its own
# positive assertions fail -- a runtime proof that the checks bite.
set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
RUN_SECONDS="${XENO_TITLE_SMOKE_SECONDS:-600}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUTDIR="${XENO_TITLE_SMOKE_OUTDIR:-$ROOT/scratchpad/title_newgame_smoke/$STAMP}"
CONTROL="${XENO_TITLE_SMOKE_CONTROL:-}"
REQUIRE_LAHAN="${XENO_TITLE_SMOKE_REQUIRE_LAHAN:-1}"
LOGFILE="$OUTDIR/run.log"
CAPDIR="$OUTDIR/captures"

mkdir -p "$OUTDIR" "$CAPDIR"
cd "$ROOT"

if [ ! -x "$BINARY" ]; then
    echo "TITLE SMOKE overall=NOT_RUN binary_missing=$BINARY" | tee "$OUTDIR/summary.txt"
    exit 2
fi
if [ ! -f disc/disc1.bin ]; then
    echo "TITLE SMOKE overall=NOT_RUN disc_missing=disc/disc1.bin" | tee "$OUTDIR/summary.txt"
    exit 2
fi

# Schedule (Vsync-shim frames; values are GAME buttons, which
# XENO_PAD_TEST_INPUT presses through the active layout's physical button --
# under the retail USA layout game Circle is the physical Cross):
#   300     Cross   skip the opening STR (state 6 allows skipping, D_8004FE47=0)
#   3700    Circle  interrupt the title attract STR inside func_800A7C58 (the
#                   attract starts near frame 3500 when the pad wait times out;
#                   an earlier Circle opens the title from the pad wait instead)
#   4500    Circle  title script pad wait -> FE57 -> title menu
#   4900    Up      Continue (1) -> New Game (2)
#   5000    Circle  confirm on release -> func_801C531C(7) entry 9 -> func_8001B970
#   5500..  Circle  2-on/2-off pulses page the Map 4 prologue (release edges)
# Re-timed 2026-09-28 (verified: full chain to Lahan map 2); the 2026-09-06
# schedule (1600/2400/2800/2900/3400) predates the retail USA remap.
SKIP_FRAME="${XENO_TITLE_SMOKE_SKIP_FRAME:-300}"
# Optional initial menu wake before the later attract skip. Current runtime
# traces need three separate Circle presses before Up/confirm; leave the
# historical schedule unchanged unless this extra slot is explicitly supplied.
PRE_ATTRACT_FRAME="${XENO_TITLE_SMOKE_PRE_ATTRACT_FRAME:-}"
ATTRACT_FRAME="${XENO_TITLE_SMOKE_ATTRACT_FRAME:-3700}"
OPEN_FRAME="${XENO_TITLE_SMOKE_OPEN_FRAME:-4500}"
UP_FRAME="${XENO_TITLE_SMOKE_UP_FRAME:-4900}"
CONFIRM_FRAME="${XENO_TITLE_SMOKE_CONFIRM_FRAME:-5000}"
PROLOGUE_FRAME="${XENO_TITLE_SMOKE_PROLOGUE_FRAME:-5500}"
# Keep two-frame presses, but optionally spread them past the opening battle
# entry without exceeding the raw-pad parser's fixed-size schedule buffer.
PROLOGUE_INTERVAL="${XENO_TITLE_SMOKE_PROLOGUE_INTERVAL:-4}"
case "$PROLOGUE_INTERVAL" in
    4|8|16|32) ;;
    *) echo "Invalid XENO_TITLE_SMOKE_PROLOGUE_INTERVAL (use 4, 8, 16 or 32)" >&2; exit 2 ;;
esac
STEP_CAP=4096

CONFIRM_BTN=0x20
if [ "$CONTROL" = "cross" ]; then
    CONFIRM_BTN=0x40
fi

schedule="0:0,${SKIP_FRAME}:0x40,$((SKIP_FRAME + 20)):0"
steps=11
if [ -n "$PRE_ATTRACT_FRAME" ]; then
    schedule+=",${PRE_ATTRACT_FRAME}:${CONFIRM_BTN},$((PRE_ATTRACT_FRAME + 2)):0"
    steps=$((steps + 2))
fi
schedule+=",${ATTRACT_FRAME}:${CONFIRM_BTN},$((ATTRACT_FRAME + 2)):0"
schedule+=",${OPEN_FRAME}:${CONFIRM_BTN},$((OPEN_FRAME + 2)):0"
schedule+=",${UP_FRAME}:0x1000,$((UP_FRAME + 2)):0"
schedule+=",${CONFIRM_FRAME}:${CONFIRM_BTN},$((CONFIRM_FRAME + 2)):0"
frame=$PROLOGUE_FRAME
while [ $((steps + 2)) -le "$STEP_CAP" ]; do
    schedule+=",${frame}:${CONFIRM_BTN},$((frame + 2)):0"
    frame=$((frame + PROLOGUE_INTERVAL))
    steps=$((steps + 2))
done
printf '%s\n' "$schedule" >"$OUTDIR/schedule.txt"
echo "TITLE SMOKE binary=$BINARY outdir=$OUTDIR steps=$steps control=${CONTROL:-none}"

set +e
env SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" \
    XENO_PAD_TEST_INPUT="$schedule" \
    XENO_VM_TRACE=0 \
    XENO_FIELD_CAPTURE_DIR="$CAPDIR" \
    XENO_FIELD_CAPTURE_EVERY="${XENO_FIELD_CAPTURE_EVERY:-120}" \
    timeout --signal=INT --kill-after=15 "${RUN_SECONDS}s" "$BINARY" \
    >"$LOGFILE" 2>&1
rc=$?
set -e
echo "TITLE SMOKE runtime_rc=$rc"

# ---- verdict ----------------------------------------------------------------
fail=0
line_of() { grep -a -n -m1 -F -- "$1" "$LOGFILE" | cut -d: -f1; }
line_after() {   # marker, after-line
    [ "$2" -gt 0 ] || return 0  # A missing prerequisite cannot restart the chain.
    grep -a -n -F -- "$1" "$LOGFILE" | awk -F: -v after="$2" '$1 > after {print $1; exit}'
}
field_after() {  # exact numeric field, after-line (0 allowed only for boot)
    grep -a -n -E -- "\[field-diag\] FieldLoad begin field=$1([[:space:]]|$)" "$LOGFILE" |
        awk -F: -v after="$2" '$1 > after {print $1; exit}'
}
report() {       # name, ok(0/1)
    if [ "$2" -eq 1 ]; then echo "TITLE SMOKE PASS $1"; else echo "TITLE SMOKE FAIL $1"; fail=1; fi
}

l490=$(field_after 490 0); l490=${l490:-0}
lfe60=$(line_after '[xeno-port][fe60] enter' "$l490"); lfe60=${lfe60:-0}
lexit=$(line_after '[xeno-port][fe60] skip Circle' "$lfe60"); lexit=${lexit:-0}
lreq=$(line_after '[xeno-port][menu] func_800799D4 request=2 D_80059460=2 -> MenuMain' "$lexit"); lreq=${lreq:-0}
ltitle=$(line_after '[xeno-port][menu] func_801C58EC title loop enter choice=1' "$lreq"); ltitle=${ltitle:-0}
lng=$(line_after '[xeno-port][menu] title confirm choice=2' "$ltitle"); lng=${lng:-0}
l4=$(if [ "$lng" -gt 0 ]; then field_after 4 "$lng"; fi); l4=${l4:-0}
l2=$(if [ "$l4" -gt 0 ]; then field_after 2 "$l4"; fi); l2=${l2:-0}
lfe57=$(line_after 'op=FE57' "$l490"); lfe57=${lfe57:-0}

positive=1
[ "$l490" -gt 0 ] || positive=0
[ "$lfe60" -gt "$l490" ] || positive=0
[ "$lexit" -gt "$lfe60" ] || positive=0
[ "$lreq" -gt "$lexit" ] || positive=0
[ "$ltitle" -gt "$lreq" ] || positive=0
[ "$lng" -gt "$ltitle" ] || positive=0
[ "$l4" -gt "$lng" ] || positive=0

fatal=0
for marker in 'missing D_8004FE50' 'ptag length is not valid' 'malloc_printerr' \
              'corrupted size' 'free(): invalid' 'double free' 'AddressSanitizer' \
              'SIGSEGV' 'SIGABRT' 'Aborted' '[stub]'; do
    if grep -a -q -F -- "$marker" "$LOGFILE"; then fatal=1; echo "TITLE SMOKE fatal marker: $marker"; fi
done

# timeout(1) returns 124 for the deliberate observation deadline. It is not
# route evidence: all required markers must still pass. Reject signals/errors,
# including 137 (forced kill), even when the log contains a complete chain.
runtime_ok=0
case "$rc" in 0|124) runtime_ok=1 ;; esac
report "runtime.normal.exit.or.observation.deadline" "$runtime_ok"

if [ "$CONTROL" = "cross" ]; then
    # Negative control: Cross must not open the title menu, so the positive
    # chain must be ABSENT. Attract entry is required, but this bounded check
    # does not claim the movie reached its end. Search the whole log for menu
    # entry: no Circle skip exists to anchor the positive-chain search here.
    control_request=$(line_of '[xeno-port][menu] func_800799D4 request=2 D_80059460=2 -> MenuMain')
    control_title=$(line_of '[xeno-port][menu] func_801C58EC title loop enter')
    report "control.reached.title.map" "$([ "$l490" -gt 0 ] && echo 1 || echo 0)"
    report "control.fe60.entered" "$([ "$lfe60" -gt 0 ] && echo 1 || echo 0)"
    report "control.cross.does.not.open.title.menu" "$([ -z "$control_request" ] && [ -z "$control_title" ] && echo 1 || echo 0)"
    report "control.positive.chain.absent" "$([ "$positive" -eq 0 ] && echo 1 || echo 0)"
    report "control.no.fatal.markers" "$([ "$fatal" -eq 0 ] && echo 1 || echo 0)"
else
    report "boot.reaches.title.map.490" "$([ "$l490" -gt 0 ] && echo 1 || echo 0)"
    report "fe60.attract.str.entered" "$([ "$lfe60" -gt "$l490" ] && echo 1 || echo 0)"
    report "fe60.circle.interrupts.attract" "$([ "$lexit" -gt "$lfe60" ] && echo 1 || echo 0)"
    report "script.reaches.fe57" "$([ "$lfe57" -gt "$l490" ] && echo 1 || echo 0)"
    report "fe57.opens.title.menu.request.2" "$([ "$lreq" -gt "$lexit" ] && echo 1 || echo 0)"
    report "title.loop.enters.on.continue" "$([ "$ltitle" -gt "$lreq" ] && echo 1 || echo 0)"
    report "up.circle.confirms.new.game" "$([ "$lng" -gt "$ltitle" ] && echo 1 || echo 0)"
    report "new.game.loads.prologue.map.4" "$([ "$l4" -gt "$lng" ] && echo 1 || echo 0)"
    if [ "$REQUIRE_LAHAN" = "1" ]; then
        report "prologue.advances.to.lahan.map.2" "$([ "$l2" -gt "$l4" ] && echo 1 || echo 0)"
    else
        echo "TITLE SMOKE INFO lahan.map.2.line=$l2 (not required)"
    fi
    report "no.fatal.markers" "$([ "$fatal" -eq 0 ] && echo 1 || echo 0)"
fi

captures=$(ls "$CAPDIR" 2>/dev/null | wc -l)
{
    echo "schedule_steps=$steps"
    echo "runtime_rc=$rc"
    echo "lines: field490=$l490 fe60=$lfe60 fe60exit=$lexit fe57=$lfe57 request2=$lreq title=$ltitle newgame=$lng field4=$l4 field2=$l2"
    echo "captures=$captures"
    if [ "$fail" -eq 0 ]; then echo "TITLE SMOKE overall=PASS"; else echo "TITLE SMOKE overall=FAIL"; fi
} | tee "$OUTDIR/summary.txt"
exit "$fail"
