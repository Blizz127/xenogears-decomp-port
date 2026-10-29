#!/usr/bin/env bash
# One dev-menu warp / step sweep target (DEV_MENU_SPEC.md 6, 7, 7a).  HEADLESS
# ONLY: offscreen GL, null audio, private XDG dirs, no X display of any kind.
#
#   warp_sweep_one.sh BIN OUTDIR CKPT MODE ARGS...
#     BIN    the xeno-port binary (run with cwd = its package dir / repo root)
#     OUTDIR per-target output (run.log, route.jsonl, frame.bmp, target.json)
#     CKPT   post-battle checkpoint (.xgqs); copied, never modified
#     MODE   warp  <map> <entrance> <scenario|-1>
#            step  <next_map> <next_ent> <addr:val,...>   (stepraw: the step delta)
#            cont  <next_map> <next_ent> <addr:val,...>   (step, then the continuation probe)
#            tablewarp <index> | tablestep   (the generated table row: `dev warp N` / `dev step`)
#   Env: XENO_DATA_DIR (disc files), SWEEP_ROOT (cwd for the binary),
#        SWEEP_IDLE (Vsync presents to idle, default 600), SWEEP_WALK (field
#        frames to hold a direction, default 240).
#
# Sequence: boot to the title, console `load` (the checkpoint puts the game in
# the post-battle field), `encounters off`, wait for the field, then the warp /
# step, then (a) arrival, (b) idle with neutral input, (c) walk-in (hold Up,
# then Right, then Down, then Left).  The analyzer (warp_sweep_analyze.py)
# judges the run.log; this script only produces the evidence.
unset DISPLAY WAYLAND_DISPLAY XAUTHORITY
set -u
BIN=$1; OUT=$2; CKPT=$3; MODE=$4; shift 4
mkdir -p "$OUT/xdg/config" "$OUT/xdg/data" "$OUT/xdg/cache" "$OUT/xdg/state" "$OUT/xdg/run" "$OUT/cap"
chmod 700 "$OUT/xdg/run"
cp "$CKPT" "$OUT/ckpt.xgqs"
ROOT=${SWEEP_ROOT:-$OUT/work}; mkdir -p "$ROOT"
LIBDIR="$(cd "$(dirname "$BIN")/../lib" 2>/dev/null && pwd)"
DATA=${XENO_DATA_DIR:?XENO_DATA_DIR must point at the disc files}
IDLE=${SWEEP_IDLE:-20}
WALK=${SWEEP_WALK:-240}
FIFO="$OUT/in.fifo"; rm -f "$FIFO"; mkfifo "$FIFO"
exec 9<>"$FIFO"
LOG="$OUT/run.log"; : >"$LOG"
cd "$ROOT" || exit 2
env ${LIBDIR:+LD_LIBRARY_PATH="$LIBDIR"} SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy ALSOFT_DRIVERS=null \
    XENO_NO_DIALOG=1 XENO_DATA_DIR="$DATA" XENO_DISC="$DATA/disc1.bin" \
    XDG_CONFIG_HOME="$OUT/xdg/config" XDG_DATA_HOME="$OUT/xdg/data" XDG_CACHE_HOME="$OUT/xdg/cache" \
    XDG_STATE_HOME="$OUT/xdg/state" XDG_RUNTIME_DIR="$OUT/xdg/run" XENO_MEMCARD_DIR="$OUT/xdg/data/memcards" \
    XENO_CHEATS_CONSOLE=1 XENO_DEV_MENU=1 XENO_DEV_MENU_UNVERIFIED=1 \
    XENO_QUICKSAVE_PATH="$OUT/ckpt.xgqs" XENO_ROUTE_LOG="$OUT/route.jsonl" \
    XENO_FIELD_POS_DIAG=30 XENO_SPEED=${XENO_SPEED:-5} \
    XENO_FIELD_CAPTURE_DIR="$OUT/cap" XENO_FIELD_CAPTURE_EVERY=${SWEEP_CAPTURE_EVERY:-240} \
    timeout --signal=TERM --kill-after=10 ${SWEEP_TIMEOUT:-420} nice -n 19 "$BIN" <"$FIFO" >>"$LOG" 2>&1 &
PID=$!
say() { echo "$*" >&9; echo "[sweep] stdin: $*" >>"$LOG"; }
waitfor() { # needle timeout_s [after_line]
    local end=$((SECONDS + $2)) from=${3:-0}
    while [ $SECONDS -lt $end ]; do
        kill -0 $PID 2>/dev/null || return 1
        if tail -n +$((from + 1)) "$LOG" | grep -aqF -- "$1"; then return 0; fi
        sleep 0.5
    done
    return 1
}
lines() { wc -l <"$LOG"; }
# wait for N more POSDIAG lines (field frames keep ticking), independent of host speed
waitpos() { # n timeout_s
    local from; from=$(lines); local end=$((SECONDS + $2))
    while [ $SECONDS -lt $end ]; do
        kill -0 $PID 2>/dev/null || return 1
        [ "$(tail -n +$((from + 1)) "$LOG" | grep -ac 'POSDIAG')" -ge "$1" ] && return 0
        sleep 0.5
    done
    return 1
}
stop() {
    kill -INT $PID 2>/dev/null
    local end=$((SECONDS + 40))
    while kill -0 $PID 2>/dev/null && [ $SECONDS -lt $end ]; do sleep 0.5; done
    kill -0 $PID 2>/dev/null && { echo "[sweep] note: game ignored SIGINT for 40 s; sending SIGTERM" >>"$LOG"; kill -TERM $PID 2>/dev/null; }
    wait $PID 2>/dev/null
}

waitfor "console reading commands" 120 || { echo "[sweep] FAIL no console" >>"$LOG"; kill $PID; exit 1; }
# Title map is loaded; the quick load is accepted from the title.
for _ in $(seq 1 30); do
    L=$(lines); say load
    waitfor "[xeno-port][quick] loading" 6 "$L" && break
    sleep 2
done
waitfor "player position restored" 120 || { echo "[sweep] FAIL checkpoint did not load" >>"$LOG"; kill $PID; exit 1; }
echo "[sweep] checkpoint loaded" >>"$LOG"
say "encounters off"
waitfor "POSDIAG" 60 || true
sleep 3
L=$(lines)
L0=$L
case "$MODE" in
    warp) say "dev warpx $1 $2 $3"; TOKEN="warpx requested" ;;
    step|cont) say "dev stepraw $1 $2 $3"; TOKEN="step finished" ;;
    tablewarp) say "dev warp $1"; TOKEN="warp requested" ;;       # the real table row (menu path)
    tablestep) say "dev step"; TOKEN="step finished" ;;           # detect predicate + table row
    tablestepx) say "dev stepx $1"; TOKEN="step finished" ;;       # table row by index (start state differs from its detect)
    *) echo "[sweep] FAIL bad mode $MODE" >>"$LOG"; kill $PID; exit 2 ;;
esac
echo "[sweep] TARGET mode=$MODE args=$*" >>"$LOG"
waitfor "$TOKEN" 120 "$L" || { echo "[sweep] FAIL action not accepted" >>"$LOG"; }
waitfor "FieldLoad begin" 90 "$L0" || echo "[sweep] FAIL no field load after the action" >>"$LOG"
echo "[sweep] ARRIVAL_WINDOW" >>"$LOG"
waitpos 6 ${SWEEP_ARRIVE:-150} || echo "[sweep] FAIL arrival: field ticks never reached 6 POSDIAG lines" >>"$LOG"
if [ "${SWEEP_PROBE:-0}" = 1 ]; then
    # discovery pass: arrival + a last frame only (no idle / walk)
    waitpos 2 60 || true; stop
    echo "[sweep] EXIT rc=$?" >>"$LOG"
    last=$(ls "$OUT/cap" 2>/dev/null | tail -n 1)
    [ -n "$last" ] && cp "$OUT/cap/$last" "$OUT/final_frame.bmp"
    rm -rf "$OUT/cap" "$OUT/xdg" "$FIFO" "$OUT/ckpt.xgqs"
    exit 0
fi
echo "[sweep] IDLE_BEGIN" >>"$LOG"
waitpos ${SWEEP_IDLE_POS:-8} 180 || echo "[sweep] FAIL idle: field ticks stopped" >>"$LOG"
echo "[sweep] IDLE_END" >>"$LOG"
for m in 0x1000 0x2000 0x4000 0x8000; do
    echo "[sweep] WALK_BEGIN $m" >>"$LOG"
    say "hold $m $WALK"
    waitpos 5 90 || true
    echo "[sweep] WALK_END $m" >>"$LOG"
done
if [ "$MODE" = cont ] || [ "$MODE" = tablestep ] || [ "$MODE" = tablestepx ]; then
    # continuation: after the step, keep playing (Circle taps and direction holds)
    # until the game itself produces the expected next event (route.jsonl line
    # matching SWEEP_CONT_REGEX) or the iteration budget is spent.
    echo "[sweep] CONT_BEGIN" >>"$LOG"
    masks=(0x20 0x20 0x1000 0x20 0x2000 0x20 0x4000 0x20 0x8000)
    for i in $(seq 1 ${SWEEP_CONT_ITER:-160}); do
        m=${masks[$((i % 9))]}
        if [ "$m" = 0x20 ]; then say "hold 0x20 4"; else say "hold $m 30"; fi
        waitpos 1 40 || break
        if [ -n "${SWEEP_CONT_REGEX:-}" ] && grep -aqE -- "$SWEEP_CONT_REGEX" "$OUT/route.jsonl" 2>/dev/null; then
            echo "[sweep] CONT_HIT after $i taps" >>"$LOG"; break
        fi
    done
    echo "[sweep] CONT_END" >>"$LOG"
fi
sleep 1
stop
rc=$?
echo "[sweep] EXIT rc=$rc" >>"$LOG"
exec 9>&-
rm -f "$FIFO" "$OUT/ckpt.xgqs"
last=$(ls "$OUT/cap" 2>/dev/null | tail -n 1)
[ -n "$last" ] && cp "$OUT/cap/$last" "$OUT/final_frame.bmp"
find "$OUT/cap" -type f ! -name "$last" -delete 2>/dev/null
rm -rf "$OUT/xdg"
exit 0
