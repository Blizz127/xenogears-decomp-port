#!/usr/bin/env bash
# Memory-card acceptance: save in the field, then title > Continue loads it.
#
# Runs pc_port/tests/card_save_load_test.py (see its docstring): run 1 quick-
# loads a field checkpoint, sets the save-point flag with the TEST TOOLING
# console command `saveflag 1` (what field opcode FE99 does inside a save
# point's area), opens the field menu and saves through the retail File >
# Save screen (func_801CBD90) to a virtual card; run 2 is a new process on the
# same card that picks Continue on the title and loads the file through the
# retail Load action (func_801CB304).  PASS: one retail-format save on the
# card, Continue leaves the title with D_800594D0=2, the saved field loads with
# the saved map, scenario flag, party HP and player position; no
# GameHandleError, no [stub].
#
#   XENO_PORT_BINARY    default pc_port/build_native/xeno-port
#   XENO_CARD_CKPT      default zz_runner/ckpt14.xgqs (field 14 checkpoint)
#   XENO_CARD_OUTDIR    default scratchpad/card_save_continue/<stamp>
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
CKPT="${XENO_CARD_CKPT:-$ROOT/zz_runner/ckpt14.xgqs}"
OUT="${XENO_CARD_OUTDIR:-$ROOT/scratchpad/card_save_continue/$(date +%Y%m%d-%H%M%S)}"
[ -x "$BINARY" ] || { echo "CARD SAVE/LOAD overall=NOT_RUN binary_missing=$BINARY"; exit 2; }
[ -f disc/disc1.bin ] || { echo "CARD SAVE/LOAD overall=NOT_RUN disc_missing=disc/disc1.bin"; exit 2; }
[ -f "$CKPT" ] || { echo "CARD SAVE/LOAD overall=NOT_RUN checkpoint_missing=$CKPT"; exit 2; }
mkdir -p "$OUT"
python3 pc_port/tests/card_save_load_test.py "$BINARY" "$OUT" "$CKPT" 2>&1 | tee "$OUT/summary.txt"
exit "${PIPESTATUS[0]}"
