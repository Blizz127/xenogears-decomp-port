#!/usr/bin/env bash
# Dev-menu cheat actions (Refill / Add Gold / Top Up Items), headless, against
# the real game state: run inside xenogears-dev after pc_port/build_port.sh.
#
# Drives the built xeno-port through the cheat console on stdin
# (XENO_CHEATS_CONSOLE=1) from a field quick-save and checks, for each action,
# that the game state is exactly what the game's own routines produce AND that
# the game is still alive afterwards: the field loop counter advances by >= N
# frames and a scripted held direction still moves the player (the owner's
# "freeze" symptom is input/loop not reaching the game).
#
# Scenarios (one process, `load` between them):
#   A  console   : hurt party -> `dev heal` -> HP/EP == max; N frames; walk
#   B  dev menu  : hurt -> F8, Cheats, Refill via the menu keys -> HP/EP == max;
#                  menu open blocks the pad; Esc closes; input returns at once;
#                  N frames; walk
#   C  game menu : field menu (Triangle) open -> `dev heal` is REFUSED with no
#                  change; close it; N frames; walk; then the same heal applies
#   D  gold/items: +10000 gold capped at MAX_GOLD_AMOUNT; owned stack +10
#                  capped at 99; N frames; walk
# Plus: the port's own stderr lines and the heartbeat reach
# "Xenogears (PC port).log" in the data dir (log mirroring).
#
# Field quick-save: $XENO_CHEATS_TEST_QUICKSAVE, else the cached
# $XENO_TEST_CACHE/cheats_field.xgqs (default ~/.cache/xeno/port), else it is
# generated from the retail boot via the title/New Game schedule + `save`
# (minutes; needs the user's disc files).  Exit 2 = NOT_RUN (no binary/disc).
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
N="${XENO_CHEATS_TEST_FRAMES:-600}"
CACHE="${XENO_TEST_CACHE:-$HOME/.cache/xeno/port}"
OUT="${XENO_CHEATS_TEST_OUTDIR:-$(mktemp -d "${TMPDIR:-/tmp}/dev-cheats.XXXXXX")}"
mkdir -p "$OUT"
LOG="$OUT/run.log"
DATALOG="$OUT/Xenogears (PC port).log"

[ -x "$BIN" ] || { echo "CHEATS TEST NOT_RUN binary_missing=$BIN"; exit 2; }
[ -f "$ROOT/disc/disc1.bin" ] || { echo "CHEATS TEST NOT_RUN disc_missing"; exit 2; }

fails=0
ok()   { echo "CHEATS TEST PASS $1"; }
bad()  { echo "CHEATS TEST FAIL $1"; fails=$((fails + 1)); }
check() { if eval "$2"; then ok "$1"; else bad "$1 [$2]"; fi; }

PID=""
start_port() {   # name, extra env...
    local name=$1; shift
    rm -f "$OUT/in.$name"; mkfifo "$OUT/in.$name"
    exec 3<>"$OUT/in.$name"
    LOG="$OUT/run.$name.log"
    : >"$LOG"
    ( cd "$OUT" && env SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless ALSOFT_DRIVERS=null \
        XENO_NO_DIALOG=1 XENO_DATA_DIR="$ROOT/disc" XENO_DISC="$ROOT/disc/disc1.bin" \
        XENO_DEV_MENU=1 XENO_CHEATS_CONSOLE=1 XENO_PAD_TEST_INPUT="${PAD_SCHEDULE:-0:0}" \
        XENO_HEARTBEAT_SECS=3 "$@" "$BIN" <"$OUT/in.$name" >"$LOG" 2>&1 ) &
    PID=$!
}
stop_port() {
    exec 3>&-
    kill -INT "$PID" 2>/dev/null; sleep 1; kill -KILL "$PID" 2>/dev/null; wait "$PID" 2>/dev/null
}
trap 'stop_port 2>/dev/null; true' EXIT
send() { echo "$1" >&3; }

wait_count() {   # pattern, count_before, timeout_s
    local t=0
    while [ "$(grep -ac -- "$1" "$LOG")" -le "$2" ]; do
        sleep 0.25; t=$((t + 1))
        [ $t -ge $(($3 * 4)) ] && return 1
        kill -0 "$PID" 2>/dev/null || return 2
    done
}
STATE=""
state() {
    local n; n=$(grep -ac '\[CHEATS\] state:' "$LOG")
    send "dev state"
    wait_count '\[CHEATS\] state:' "$n" 30 || { STATE=""; return 1; }
    STATE=$(grep -a '\[CHEATS\] state:' "$LOG" | tail -1)
}
field() { sed -E "s/.* $1=([^ ]*).*/\1/" <<<"$STATE"; }
party_hp()  { field party | cut -d';' -f1 | sed -E 's/^[0-9]+:([0-9]+)\/([0-9]+),([0-9]+)\/([0-9]+)$/\1/'; }
party_max() { field party | cut -d';' -f1 | sed -E 's/^[0-9]+:([0-9]+)\/([0-9]+),([0-9]+)\/([0-9]+)$/\2/'; }
party_mp()  { field party | cut -d';' -f1 | sed -E 's/^[0-9]+:([0-9]+)\/([0-9]+),([0-9]+)\/([0-9]+)$/\3/'; }
party_mmp() { field party | cut -d';' -f1 | sed -E 's/^[0-9]+:([0-9]+)\/([0-9]+),([0-9]+)\/([0-9]+)$/\4/'; }
all_full() {     # every party member at max HP and EP
    local m bad=0
    for m in $(field party | tr ';' ' '); do
        IFS=':,/' read -r _ hp mhp mp mmp <<<"$m"
        [ "$hp" = "$mhp" ] && [ "$mp" = "$mmp" ] || bad=1
    done
    [ $bad = 0 ] && [ -n "$(field party)" ]
}
wait_safe() {    # wait for a free field frame
    local t=0
    while :; do
        state && [ "$(field safe)" = 1 ] && [ "$(field pos)" != none ] && return 0
        sleep 1; t=$((t + 1)); [ $t -ge ${1:-60} ] && return 1
    done
}
do_load() {
    local n; n=$(grep -ac 'player position restored' "$LOG")
    send load
    wait_count 'player position restored' "$n" 90 || return 1
    wait_safe 60
}
advance() {      # N field-loop passes must elapse (freeze check)
    local start end t=0
    state || return 1; start=$(field loop)
    while :; do
        state || return 1; end=$(field loop)
        [ $((end - start)) -ge "$1" ] && return 0
        sleep 1; t=$((t + 1)); [ $t -ge 240 ] && return 1
    done
}
walk() {         # held direction $1 must move the player
    local p0 p1 t=0
    state || return 1; p0=$(field pos)
    send "dev pad $1"
    while :; do
        sleep 1; state || return 1; p1=$(field pos)
        [ "$p1" != "$p0" ] && break
        t=$((t + 1)); [ $t -ge 15 ] && break
    done
    send "dev pad 0"
    [ "$p1" != "$p0" ]
}

# ---- field quick-save --------------------------------------------------------
QS="${XENO_CHEATS_TEST_QUICKSAVE:-$CACHE/cheats_field.xgqs}"
if [ ! -s "$QS" ]; then
    echo "CHEATS TEST generating a field quick-save -> $QS (title/New Game schedule)"
    mkdir -p "$(dirname "$QS")"
    # Same schedule as run_title_newgame_smoke.sh (Vsync-shim frames, game
    # buttons): Cross skips the STR, Circle opens the title, Up -> New Game,
    # Circle confirms, then Circle pulses page the prologue.
    sched="0:0,300:0x40,302:0,3700:0x20,3702:0,4500:0x20,4502:0,4900:0x1000,4902:0,5000:0x20,5002:0"
    f=5500; for _ in $(seq 1 1500); do sched+=",$f:0x20,$((f + 2)):0"; f=$((f + 4)); done
    PAD_SCHEDULE="$sched" start_port gen XENO_PAD_TEST_STOP_FIELD=2 XENO_QUICKSAVE_PATH="$QS"
    t=0; until grep -aq 'FieldLoad begin field=2\b' "$LOG"; do
        sleep 2; t=$((t + 2)); kill -0 "$PID" 2>/dev/null || break; [ $t -ge 1500 ] && break; done
    if wait_safe 300; then
        n=$(grep -ac 'quick\] saved' "$LOG"); send save
        wait_count 'quick\] saved' "$n" 120 || true
    fi
    stop_port
    [ -s "$QS" ] || { echo "CHEATS TEST NOT_RUN could not generate a field quick-save"; exit 2; }
fi
cp "$QS" "$OUT/quick.xgqs"
PAD_SCHEDULE=0:0 start_port main XENO_QUICKSAVE_PATH="$OUT/quick.xgqs"

# Reach the title field, then load the field quick-save.
t=0; until grep -aq 'FieldLoad begin field=490' "$LOG"; do
    sleep 1; t=$((t + 1)); kill -0 "$PID" 2>/dev/null || break; [ $t -ge 300 ] && break; done
sleep 3

# ---- A: console refill --------------------------------------------------------
echo "== A: console refill"
check "A load quick-save -> free field" "do_load"
send "dev hurt"; state
check "A party hurt (HP=1 EP=0)" '[ "$(party_hp)" = 1 ] && [ "$(party_mp)" = 0 ]'
send "dev heal"; sleep 1; state
check "A refill: every party member HP/EP == max" "all_full"
check "A refill: HP equals max ($(party_hp)/$(party_max))" '[ "$(party_hp)" = "$(party_max)" ] && [ "$(party_mp)" = "$(party_mmp)" ]'
check "A +$N frames: field loop advances" "advance $N"
check "A held Up still moves the player" "walk 0x1000"
check "A input gate is the game's" '[ "$(field input)" = game ]'

# ---- B: dev menu path ---------------------------------------------------------
echo "== B: dev menu Cheats > Refill"
check "B load" "do_load"
send "dev hurt"; state
check "B party hurt" '[ "$(party_hp)" = 1 ]'
for k in f8 down down enter down down enter; do send "dev key $k"; sleep 0.6; done
sleep 1; state
check "B refill through the menu: HP/EP == max" "all_full"
check "B menu open blocks the pad" '[ "$(field input)" = menu ]'
send "dev key esc"; sleep 0.6; send "dev key esc"; sleep 1; state
check "B closing the menu returns input to the game (no stuck drain)" '[ "$(field input)" = game ]'
check "B +$N frames: field loop advances" "advance $N"
check "B held Right still moves the player" "walk 0x2000"

# ---- C: refill with the game's field menu open --------------------------------
echo "== C: Refill while the field menu is open"
check "C load" "do_load"
send "dev hurt"; state
send "dev pad 0x10"; sleep 1; send "dev pad 0"; sleep 3; state
check "C field menu is open (not a free frame)" '[ "$(field safe)" = 0 ]'
n=$(grep -ac "refused 'heal'" "$LOG")
send "dev heal"; wait_count "refused 'heal'" "$n" 10; state
check "C refill refused while the menu is open" '[ "$(party_hp)" = 1 ]'
send "dev pad 0x40"; sleep 1; send "dev pad 0"
check "C closing the field menu returns to a free field" "wait_safe 30"
check "C +$N frames: field loop advances" "advance $N"
check "C held Left still moves the player" "walk 0x8000"
send "dev heal"; sleep 1; state
check "C refill applies on the free frame" "all_full"

# ---- D: gold and items --------------------------------------------------------
echo "== D: gold and items"
check "D load" "do_load"
state; g0=$(field gold)
send "dev gold"; sleep 1; state
check "D gold +10000 ($g0 -> $(field gold))" '[ "$(field gold)" = $((g0 + 10000)) ]'
send "dev setgold 9995000"; sleep 0.5; send "dev gold"; sleep 1; state
check "D gold capped at MAX_GOLD_AMOUNT 9999999" '[ "$(field gold)" = 9999999 ]'
send "dev seed 1:95"; sleep 0.5; state; s0=$(field itemSlots)
send "dev items"; sleep 1; state
check "D owned stack 95 -> capped at 99" '[ "$(field itemTotal)" = 99 ] && [ "$(field itemSlots)" = "$s0" ]'
send "dev seed 2:5"; sleep 0.5; send "dev items"; sleep 1; state
check "D owned stack 5 -> 15 (+10), cap respected, no new slot" '[ "$(field itemTotal)" = 114 ]'
check "D +$N frames: field loop advances" "advance $N"
check "D held Down still moves the player" "walk 0x4000"

# ---- log mirroring + heartbeat -------------------------------------------------
echo "== log mirror / heartbeat"
sleep 4
stop_port
check "log file exists in the data dir" '[ -s "$DATALOG" ]'
for tag in '\[DEV_MENU\]' '\[CHEATS\]' '\[xeno-port\]' '\[xeno-port\]\[heartbeat\]'; do
    check "log file holds $tag lines" 'grep -aq -- "$tag" "$DATALOG"'
done
check "heartbeat carries frames/map/field-safe/input-drain" \
    'grep -aqE "heartbeat\] t=[0-9]+s frames=[0-9]+ fieldLoop=[0-9]+ map=[0-9]+ fieldSafe=[01] input=[a-z-]+ drainMs=[0-9]+" "$DATALOG"'

if [ "$fails" -eq 0 ]; then echo "CHEATS TEST OVERALL PASS (logs in $OUT)"; exit 0; fi
echo "CHEATS TEST OVERALL FAIL: $fails check(s) (logs in $OUT)"
exit 1
