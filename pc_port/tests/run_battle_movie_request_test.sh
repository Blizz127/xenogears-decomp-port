#!/usr/bin/env bash
# TEST TOOLING: the post-battle movie request end to end on a hybrid binary.
# battle_command_file1 func_801E77E4's stores (movie request + end frame) are
# replayed through the production battle bus during battle 1 (LD_PRELOAD
# battle_movie_request_probe.c), together with the D_800D3338 flag the battle
# exit func_8001B6C4 reads to call ChangeGameState(6) (battle 1 is a random
# battle: at its return the probe sets a non-win result byte to 1, as retail's
# event battles leave it, so the exit consults the flag); the probe log shows the
# state changes, MovieMain's type / number / end frame and the return to the
# field.  Movie frames (boot movie set aside in movie-frames/boot) and field
# frames are captured at 1280x960.
#
#   run_battle_movie_request_test.sh BINARY OUTDIR [TYPE:NUMBER:END:NOSKIP]
#
# The default request is the one retail's battle event scripts make: archive
# 0x20 file 2, encounter 3 (D_8006F9DF), script +0x163, after-controller
# opcode 0x27 with immediates 1, 5, 1, 608 (find_battle_movie_requests.py).
#
# Battle 1 is replayed the way the seed fixture does it
# (run_battle_fault_probe.sh): battle_fault_probe_preload.c, chained after the
# movie probe, forces the rand state MOVIE_SEED (default 1c47d175; "" for an
# unseeded battle) at battle entry under the deterministic vblank clock, so the
# battle runs a fixed length for a given binary and the seed log
# (OUTDIR/seed.log) shows D_800C48EA / D_800D3338 per battle frame.  Release vs
# native counts differ: the release line (r6.18) returns after 22290472
# interpreted instructions, 22290445 with the request stored (the battle's end
# sequence reads it); the native line runs more of the battle as native leaves
# (17291921 with the request stored, merge 09bc8a1c).
#
# Needs the field-14 checkpoint (XENO_ASSISTS_CKPT).  OUTDIR under scratch.
set -euo pipefail
BIN=$(readlink -f "$1"); mkdir -p "$2"; OUT=$(readlink -f "$2"); REQ=${3:-1:5:608:1}
HERE=$(cd "$(dirname "$0")" && pwd)
SEED=${MOVIE_SEED-1c47d175}
gcc -shared -fPIC -O2 -Wall -o "$OUT/probe.so" "$HERE/battle_movie_request_probe.c" -ldl -lpthread
gcc -shared -fPIC -O2 -o "$OUT/seed.so" "$HERE/battle_fault_probe_preload.c" -ldl -lpthread
sym() { nm "$BIN" | awk -v s="$1" '$3 == s { print $1; exit }'; }
for v in g_ActiveBattleRuntime runtime_write g_CurGameState g_PsxRam D_80077448 \
         D_8007711C D_8007739C D_8004FE44 D_8004FE45 D_8004FE46 D_8004FE47 D_80062514 \
         g_RandomSeed battle.3; do
    [ -n "$(sym $v)" ] || { echo "MOVIE REQUEST FAIL no $v in $BIN"; exit 2; }
done
FRAME=$(printf '%x' $(( 0x$(sym battle.3) + 4 )))   # PcPort_BattleStateHashBeforeGuest's frame
nm "$BIN" | awk -v a="$FRAME" '$1 ~ a"$" && $3 ~ /^frame\./ { ok=1 } END { exit !ok }' ||
    { echo "MOVIE REQUEST FAIL frame static not at battle.3+4 in $BIN"; exit 2; }
mkdir -p "$OUT/movie-frames" "$OUT/field-frames"
export XENO_ASSISTS_CKPT=${XENO_ASSISTS_CKPT:?set XENO_ASSISTS_CKPT to the field-14 checkpoint}
timeout "${MOVIE_RUN_TIMEOUT:-1500}" python3 "$HERE/battle_movie_request_driver.py" "$BIN" "$OUT/run" \
    "$OUT/probe.log" "${MOVIE_WAIT:-600}" \
    LD_PRELOAD="$OUT/probe.so $OUT/seed.so" MRP_LOG="$OUT/probe.log" MRP_REQUEST="$REQ" \
    MRP_RUNTIME="$(sym g_ActiveBattleRuntime)" MRP_WRITE="$(sym runtime_write)" \
    MRP_STATE="$(sym g_CurGameState)" MRP_RAM="$(sym g_PsxRam)" \
    MRP_TYPE="$(sym D_80077448)" MRP_NUMBER="$(sym D_8007711C)" MRP_END="$(sym D_8007739C)" \
    MRP_FE44="$(sym D_8004FE44)" MRP_FE45="$(sym D_8004FE45)" MRP_FE46="$(sym D_8004FE46)" \
    MRP_FE47="$(sym D_8004FE47)" MRP_F2514="$(sym D_80062514)" \
    PROBE_LOG="$OUT/seed.log" PROBE_PSXRAM="$(sym g_PsxRam)" PROBE_SEED_ADDR="$(sym g_RandomSeed)" \
    PROBE_FRAME_ADDR="$FRAME" PROBE_GUEST="c48ea:800C48EA:1,d3338:800D3338:1" PROBE_SEED="$SEED" \
    XENO_DETERMINISTIC_VBLANK=2 XENO_DETERMINISTIC_VBLANK_FROM_FIELD=14 \
    XENO_DETERMINISTIC_VBLANK_BATTLE=1 XENO_SOUND_TICK_LOCKSTEP=1 \
    XENO_CPU_FALLBACK_TOP=all XENO_CPU_FALLBACK_STATS=1 \
    XENO_VIDEO_WIDTH=1280 XENO_VIDEO_HEIGHT=960 \
    XENO_MOVIE_CAPTURE_DIR="$OUT/movie-frames" XENO_MOVIE_CAPTURE_FRAMES="${MOVIE_FRAMES:-5,150,300,600}" \
    XENO_FIELD_CAPTURE_DIR="$OUT/field-frames" XENO_FIELD_CAPTURE_EVERY="${FIELD_EVERY:-120}"
