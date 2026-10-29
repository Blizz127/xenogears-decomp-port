#!/usr/bin/env bash
# HD-2D OFF-IS-IDENTICAL gate.
#
# Runs the same fixed-input headless window -- the title/New Game schedule of
# run_title_newgame_smoke.sh up to schedule frame N (default 3000) -- with two
# links of the SAME build, logging every presented frame's CRC-32
# (XENO_PRESENT_HASH_LOG, psycross_hd2d_hooks.patch), and compares them.
#
#   test mode (the build defines no HD-2D hook, e.g. only hd2d_stub.c):
#     A = pc_port/build_native/xeno-port as built
#     B = A's link inputs + pc_port/tests/hd2d_off_identity_hooks.c, a
#         pc_port/src/hd2d-style file that defines every hook, all off
#   lane mode (pc_port/src/hd2d defines hooks, e.g. the Codex lane):
#     A = the build's link inputs WITHOUT any pc_port/src/hd2d object
#     B = pc_port/build_native/xeno-port as built (the lane, left off)
#
# The port is paced by the wall clock (vblank thread, CD streaming), so on a
# loaded box two runs of ONE binary need not show the same frame at the same
# present index. Each logged present therefore also carries an input key: a
# CRC of everything it was rendered from (draw batches + VRAM mirror, or the
# VRAM display rows for a VRAM present). pc_port/tests/hd2d_present_hash_compare.py
# passes when every present key seen in both runs has the same frame CRC in
# both (no exceptions), and enough of each run pairs up that way: at least
# XENO_HD2D_OFF_TEST_MIN_PAIRED_S (90) percent of scene presents and
# XENO_HD2D_OFF_TEST_MIN_PAIRED_V (40) percent of VRAM presents (mostly STR
# movie frames, which the wall-clock CD pacing samples differently each run),
# or when the logs are identical index for index. On a loaded box the same
# schedule can also take a different game path (the boot STR ending before or
# after the frame-300 skip); such an A/B pair is re-run, up to
# XENO_HD2D_OFF_TEST_ATTEMPTS (4) times, and never counted as a pass.
# XENO_HD2D_OFF_TEST_CONTROL=1 adds a control run (A again);
# XENO_HD2D_OFF_TEST_MUTANT=1 links B with a hook that paints a 4x4 block at
# present, which must FAIL (proves the gate bites).
#
# Needs a finished pc_port/build_port.sh (it records xeno-port.link-args and
# port-source.cflags). Run from the repository (host or xenogears-dev):
#   bash pc_port/tests/run_hd2d_off_identity_test.sh
# Env: XENO_HD2D_OFF_TEST_FRAMES (3000), XENO_HD2D_OFF_TEST_OUTDIR,
#      XENO_HD2D_OFF_TEST_MODE=test|lane (auto), XENO_HD2D_OFF_TEST_SPEED (1),
#      XENO_HD2D_OFF_TEST_SECONDS per run (300).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT_BUILD=pc_port/build_native
BIN="$OUT_BUILD/xeno-port"
FRAMES="${XENO_HD2D_OFF_TEST_FRAMES:-3000}"
SPEED="${XENO_HD2D_OFF_TEST_SPEED:-1}"
MIN_PAIRED_S="${XENO_HD2D_OFF_TEST_MIN_PAIRED_S:-90}"
MIN_PAIRED_V="${XENO_HD2D_OFF_TEST_MIN_PAIRED_V:-40}"
ATTEMPTS="${XENO_HD2D_OFF_TEST_ATTEMPTS:-4}"
# 600 s like the smoke's own default: since the world map's VSync(n>=2) fix the
# retail pacing makes 3000 schedule frames take close to 300 s on a loaded box,
# and a run cut by the time limit one frame short reads as "did not reach".
SECONDS_PER_RUN="${XENO_HD2D_OFF_TEST_SECONDS:-600}"
OUTDIR="${XENO_HD2D_OFF_TEST_OUTDIR:-$ROOT/scratchpad/hd2d_off_identity/$(date +%Y%m%d-%H%M%S)}"
mkdir -p "$OUTDIR"
OUTDIR="$(cd "$OUTDIR" && pwd)"
say() { echo "HD2D OFF-IDENTITY $*" | tee -a "$OUTDIR/summary.txt"; }

for f in "$BIN" "$OUT_BUILD/xeno-port.link-args" "$OUT_BUILD/port-source.cflags" disc/disc1.bin; do
    if [ ! -e "$f" ]; then say "overall=NOT_RUN missing=$f (run pc_port/build_port.sh)"; exit 2; fi
done
if ! grep -q 'XENO_PRESENT_HASH_LOG' pc_port/extern/PsyCross/src/PsyX_main.cpp; then
    say "overall=NOT_RUN psycross_hd2d_hooks.patch not applied"; exit 2
fi

HOOKS="PcPort_Hd2dPostProcess PcPort_Hd2dTextureOverride PcPort_Hd2dSpriteBegin PcPort_Hd2dSpriteDraw PcPort_Hd2dSpriteEnd"
defined_hooks() { nm --defined-only "$1" 2>/dev/null | awk '{print $3}' | grep -xE "$(echo $HOOKS | tr ' ' '|')" | sort | tr '\n' ' '; }
MODE="${XENO_HD2D_OFF_TEST_MODE:-}"
if [ -z "$MODE" ]; then
    if [ -n "$(defined_hooks "$BIN")" ]; then MODE=lane; else MODE=test; fi
fi

mapfile -t LINK_ARGS < "$OUT_BUILD/xeno-port.link-args"
A="$OUTDIR/xeno-port.A"
B="$OUTDIR/xeno-port.B"
case "$MODE" in
    test)
        cp "$BIN" "$A"
        # shellcheck disable=SC2046
        mutant=""
        [ "${XENO_HD2D_OFF_TEST_MUTANT:-0}" = 1 ] && mutant=-DHD2D_OFF_TEST_MUTANT
        if ! gcc -c pc_port/tests/hd2d_off_identity_hooks.c $(cat "$OUT_BUILD/port-source.cflags") $mutant \
                -o "$OUTDIR/hd2d_off_identity_hooks.o" 2>"$OUTDIR/hooks.err" ||
           ! gcc "$OUTDIR/hd2d_off_identity_hooks.o" "${LINK_ARGS[@]}" -o "$B" 2>"$OUTDIR/linkB.err"; then
            say "overall=FAIL could not build B (see $OUTDIR/hooks.err, linkB.err)"; exit 1
        fi ;;
    lane)
        cp "$BIN" "$B"
        filtered=()
        for a in "${LINK_ARGS[@]}"; do case "$a" in */obj/hd2d__*.o) ;; *) filtered+=("$a") ;; esac; done
        if ! gcc "${filtered[@]}" -o "$A" 2>"$OUTDIR/linkA.err"; then
            say "overall=FAIL could not link A without the hd2d objects (see $OUTDIR/linkA.err)"; exit 1
        fi ;;
    *) say "overall=NOT_RUN bad XENO_HD2D_OFF_TEST_MODE=$MODE"; exit 2 ;;
esac
say "mode=$MODE frames=$FRAMES speed=${SPEED}x A_hooks=[$(defined_hooks "$A")] B_hooks=[$(defined_hooks "$B")]"
if [ -n "$(defined_hooks "$A")" ] || [ -z "$(defined_hooks "$B")" ]; then
    say "overall=FAIL A must define no hook and B must define hooks"; exit 1
fi

run_one() { # name binary
    local name="$1" bin="$2" n last
    rm -rf "$OUTDIR/run_$name" "$OUTDIR/hash_$name.txt"
    # XENO_LOG_TEE=0: the stderr tee thread (port_log.c) is not drained by the
    # present-hash _Exit, which could drop the "presents logged; exiting" marker.
    XENO_LOG_TEE=0 ALSOFT_DRIVERS=null SDL_AUDIODRIVER=dummy \
    XENO_PORT_BINARY="$bin" \
    XENO_TITLE_SMOKE_OUTDIR="$OUTDIR/run_$name" \
    XENO_TITLE_SMOKE_SECONDS="$SECONDS_PER_RUN" \
    XENO_FIELD_CAPTURE_EVERY=1000000 \
    XENO_PRESENT_HASH_LOG="$OUTDIR/hash_$name.txt" \
    XENO_PRESENT_HASH_UNTIL_FRAME="$FRAMES" \
    XENO_SPEED="$SPEED" \
        bash pc_port/tests/run_title_newgame_smoke.sh >"$OUTDIR/smoke_$name.txt" 2>&1
    n=$(wc -l < "$OUTDIR/hash_$name.txt" 2>/dev/null || echo 0)
    last=$(tail -1 "$OUTDIR/hash_$name.txt" 2>/dev/null | awk '{print $6}')
    say "run=$name presents=$n last_frame=${last:-none} reached_frame_$FRAMES=$(grep -a -c 'presents logged; exiting' "$OUTDIR/run_$name/run.log")"
}
# The game path under the fixed schedule: the boot movie-state changes and
# field loads, in order. On a loaded box the wall-clock STR can end on its own
# before the frame-300 skip, after which the same schedule drives the boot
# somewhere else (another movie, no title map inside the window); frames are
# compared only between runs that took the same path (a different path is
# retried, never counted as a pass).
path_signature() {
    grep -a -o -E '\[xeno-port\]\[boot\] retail boot: movie state [0-9]+[^>]*-> state [0-9]+|FieldLoad begin field=[0-9]+' \
        "$OUTDIR/run_$1/run.log" 2>/dev/null | tr '\n' '|'
}

fail=0
attempt=1
while :; do
    run_one A "$A"
    run_one B "$B"
    sa=$(path_signature A); sb=$(path_signature B)
    if [ "$sa" = "$sb" ]; then say "attempt=$attempt same game path in A and B"; break; fi
    say "attempt=$attempt A and B took different game paths (wall-clock timing); retrying"
    for r in A B; do
        cp "$OUTDIR/run_$r/run.log" "$OUTDIR/attempt${attempt}_$r.log" 2>/dev/null
        cp "$OUTDIR/hash_$r.txt" "$OUTDIR/attempt${attempt}_hash_$r.txt" 2>/dev/null
    done
    attempt=$((attempt + 1))
    if [ "$attempt" -gt "$ATTEMPTS" ]; then say "FAIL no attempt out of $ATTEMPTS gave A and B the same game path"; fail=1; break; fi
done
[ "${XENO_HD2D_OFF_TEST_CONTROL:-0}" = 1 ] && run_one A2 "$A"

for r in A B; do
    if ! grep -a -q 'presents logged; exiting' "$OUTDIR/run_$r/run.log"; then
        say "FAIL run $r did not reach schedule frame $FRAMES"; fail=1
    fi
done
hooks_line=$(grep -a '\[hd2d-off-hooks\]' "$OUTDIR/run_B/run.log" 2>/dev/null | tail -1)
[ -n "$hooks_line" ] && say "B ${hooks_line#*] }"
if [ "$MODE" = test ]; then
    post=$(echo "$hooks_line" | sed -n 's/.*post=\([0-9]*\).*/\1/p')
    tex=$(echo "$hooks_line" | sed -n 's/.* tex=\([0-9]*\).*/\1/p')
    if [ "${post:-0}" -eq 0 ] || [ "${tex:-0}" -eq 0 ]; then
        say "FAIL the test hooks were not exercised (post=${post:-0} tex=${tex:-0})"; fail=1
    fi
fi
python3 pc_port/tests/hd2d_present_hash_compare.py "$OUTDIR/hash_A.txt" "$OUTDIR/hash_B.txt" \
    "$MIN_PAIRED_S" "$MIN_PAIRED_V" "A-vs-B" | tee -a "$OUTDIR/summary.txt"
[ "${PIPESTATUS[0]}" -eq 0 ] || fail=1
if [ -f "$OUTDIR/hash_A2.txt" ]; then
    [ "$(path_signature A)" = "$(path_signature A2)" ] || say "control A2 took a different game path (informational)"
    python3 pc_port/tests/hd2d_present_hash_compare.py "$OUTDIR/hash_A.txt" "$OUTDIR/hash_A2.txt" \
        "$MIN_PAIRED_S" "$MIN_PAIRED_V" "control:A-vs-A2" | tee -a "$OUTDIR/summary.txt"
fi
if [ "$fail" -eq 0 ]; then say "overall=PASS outdir=$OUTDIR"; else say "overall=FAIL outdir=$OUTDIR"; fi
exit "$fail"
