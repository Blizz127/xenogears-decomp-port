#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT_ROOT="${W34C1_INPUT_ACCEPT_DIR:-$ROOT/pc_port/build_native/w34c1_scripted_input_acceptance}"
GDB_SCRIPT="$ROOT/pc_port/tests/w34c1_scripted_input_detach.gdb"
BINARY="$ROOT/pc_port/build_native/xeno-port"
BOOTSTRAP_SCHEDULE='0:0x2000,600:0x4000,916:0'
MOVEMENT_SCHEDULE='0:0x2000,600:0x4000,916:0x2000,1036:0'

mkdir -p "$OUT_ROOT"
RUN_ROOT="$(mktemp -d "$OUT_ROOT/run.XXXXXX")"

run_one() {
    local label="$1"
    local schedule="$2"
    local capture="$RUN_ROOT/$label"
    local log="$RUN_ROOT/$label.log"
    local pid
    local count

    mkdir -p "$capture"
    XENO_TEST_INPUT="$schedule" XENO_CAPTURE_DIR="$capture" \
        timeout 180s gdb -q -batch -x "$GDB_SCRIPT" --args "$BINARY" \
        >"$log" 2>&1
    pid="$(sed -n 's/.*W34C1_SCRIPTED_INPUT_DETACH pid=\([0-9][0-9]*\).*/\1/p' \
        "$log" | tail -1)"
    if [[ -z "$pid" ]]; then
        echo "ASSERTION $label.detach_pid" >&2
        return 1
    fi

    count=0
    while ! rg -q '^\[worldmap-open-loop\] returned to init$' "$log"; do
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "ASSERTION $label.exited_before_bounded_return" >&2
            return 1
        fi
        sleep 1
        count=$((count + 1))
        if [[ "$count" -ge 180 ]]; then
            echo "ASSERTION $label.native_timeout" >&2
            kill -9 "$pid" 2>/dev/null || true
            return 1
        fi
    done
    kill "$pid" 2>/dev/null || true
    sleep 1
    if kill -0 "$pid" 2>/dev/null; then
        kill -9 "$pid" 2>/dev/null || true
    fi

    rg -q '^\[worldmap-open-loop\] frame=60/120$' "$log"
    rg -q '^\[worldmap-open-loop\] frame=120/120$' "$log"
    rg -q 'fulfilled request_frame=60 fulfillment_frame=60' "$log"
    rg -q 'fulfilled request_frame=120 fulfillment_frame=120' "$log"
    test -s "$capture/world-frame-000060.bmp"
    test -s "$capture/world-frame-000120.bmp"
    echo "$label PASS capture=$capture"
}

run_one same_a "$MOVEMENT_SCHEDULE"
run_one same_b "$MOVEMENT_SCHEDULE"
run_one no_world_input "$BOOTSTRAP_SCHEDULE"

for frame in 000060 000120; do
    cmp "$RUN_ROOT/same_a/world-frame-$frame.bmp" \
        "$RUN_ROOT/same_b/world-frame-$frame.bmp"
    if cmp -s "$RUN_ROOT/same_a/world-frame-$frame.bmp" \
        "$RUN_ROOT/no_world_input/world-frame-$frame.bmp"; then
        echo "ASSERTION movement.frame_$frame.differs_from_no_input" >&2
        exit 1
    fi
done

sha256sum "$RUN_ROOT"/*/world-frame-*.bmp
echo "W34C1 RUNG2 DETACHED ACCEPTANCE PASS run_root=$RUN_ROOT"
