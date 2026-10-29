#!/usr/bin/env bash
# Soft reset acceptance: START+SELECT+L1+R1 in the field restarts the port.
# pc_port/src/port_soft_reset.c is the port's equivalent of retail's crt0
# restart (GameSoftReset, src/slus_006.64/main/main_loop.c:199).
#
# Runs pc_port/tests/soft_reset_test.py (see its docstring). It boots headless,
# quick-loads a field checkpoint and holds START+SELECT+L1+R1 (CTRL_BTN_*
# 0x090C) through the TEST TOOLING console. Then it requires:
#   - the same PID re-execs and boots again (ArchiveInit, title field 490);
#   - the restarted image reaches the title menu;
#   - the inherited descriptors and the process name are unchanged;
#   - no GameHandleError, no crash and no [stub] other than CdFlush.
# Capped at 600 s.
#
#   XENO_PORT_BINARY        default pc_port/build_native/xeno-port
#   XENO_SOFT_RESET_CKPT    default zz_runner/ckpt14.xgqs (field 14 checkpoint)
#   XENO_SOFT_RESET_OUTDIR  default scratchpad/soft_reset/<stamp>
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
CKPT="${XENO_SOFT_RESET_CKPT:-$ROOT/zz_runner/ckpt14.xgqs}"
OUT="${XENO_SOFT_RESET_OUTDIR:-$ROOT/scratchpad/soft_reset/$(date +%Y%m%d-%H%M%S)}"
[ -x "$BINARY" ] || { echo "SOFT RESET overall=NOT_RUN binary_missing=$BINARY"; exit 2; }
[ -f disc/disc1.bin ] || { echo "SOFT RESET overall=NOT_RUN disc_missing=disc/disc1.bin"; exit 2; }
[ -f "$CKPT" ] || { echo "SOFT RESET overall=NOT_RUN checkpoint_missing=$CKPT"; exit 2; }
mkdir -p "$OUT"
timeout -k 15 600 python3 pc_port/tests/soft_reset_test.py "$BINARY" "$OUT" "$CKPT" 2>&1 \
    | tee "$OUT/summary.txt"
rc=${PIPESTATUS[0]}
[ "$rc" = 124 ] && echo "SOFT RESET overall=FAIL timeout=600s" | tee -a "$OUT/summary.txt"
exit "$rc"
