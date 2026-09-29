#!/usr/bin/env bash
# W18I world forbidden-instrumentation validation suite.
#
# Runs the hardened harnesses on the host graphical session (DISPLAY :10,
# never Docker), captures logs under scratchpad/w18i_forbidden_instrumentation/,
# and checks every strict summary with check_forbidden_instrumentation.py.
#
# Usage (repo root):
#   bash pc_port/tools/world_harness/run_w18i.sh
# Exit: 0 when every component passes, nonzero otherwise.
set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT="$ROOT/scratchpad/w18i_forbidden_instrumentation"
HARNESS="$ROOT/pc_port/tools/world_harness"
CHECKER="$ROOT/pc_port/tools/check_forbidden_instrumentation.py"
BIN="$ROOT/pc_port/build_native/xeno-port"
TIMEOUT="${W18I_TIMEOUT:-290}"

export DISPLAY="${DISPLAY:-:10}"

if [ ! -x "$BIN" ] && [ ! -f "$BIN" ]; then
    echo "ERROR: $BIN missing — run tools/ovh/native-build first" >&2
    exit 2
fi
mkdir -p "$OUT"

overall=0

run_harness() {
    # run_harness <name> <script> <expected-exit> [ENV=VAL ...]
    local name="$1" script="$2" expect="$3"; shift 3
    local log="$OUT/$name.log"
    echo "=== $name (expect exit $expect) ==="
    env "$@" timeout -s KILL "$TIMEOUT" gdb -batch -x "$script" \
        --args "$BIN" > "$log" 2>&1
    local rc=$?
    if [ "$rc" -ne "$expect" ]; then
        echo "FAIL $name: exit $rc != expected $expect (log: $log)"
        overall=1
        return 1
    fi
    echo "OK $name: exit $rc"
    return 0
}

check_log() {
    # check_log <name> <expected-checker-exit>
    local name="$1" expect="$2"
    local log="$OUT/$name.log"
    if python3 "$CHECKER" "$log" > "$OUT/$name.check" 2>&1; then
        local rc=0
    else
        local rc=$?
    fi
    if [ "$rc" -ne "$expect" ]; then
        echo "FAIL checker($name): exit $rc != expected $expect"
        cat "$OUT/$name.check"
        overall=1
        return 1
    fi
    echo "OK checker($name): $(tail -1 "$OUT/$name.check")"
    return 0
}

# --- checker unit fixtures -------------------------------------------------
python3 "$ROOT/pc_port/tools/test_check_forbidden_instrumentation.py" \
    > "$OUT/checker_unit.log" 2>&1
if [ $? -ne 0 ]; then
    echo "FAIL checker unit fixtures (log: $OUT/checker_unit.log)"
    overall=1
else
    echo "OK checker unit fixtures"
fi

# --- W18B natural + checker ------------------------------------------------
if run_harness natural "$HARNESS/w18b_natural.gdb" 0; then
    check_log natural 0 || true
fi

# --- W18B hold-enabled + checker --------------------------------------------
if run_harness hold_enabled "$HARNESS/w18b_hold_enabled.gdb" 0; then
    check_log hold_enabled 0 || true
fi

# --- gate-off smoke + checker -----------------------------------------------
if run_harness gate_off "$HARNESS/w18b_gate_off.gdb" 0; then
    check_log gate_off 0 || true
fi

# --- negative controls: every scenario must exit nonzero --------------------
for sc in unregistered_required bad_symbol missing_counter disabled_bp \
          deleted_bp hit_required; do
    run_harness "ctrl_$sc" "$HARNESS/w18b_controls.gdb" 1 \
        "XENO_W18I_CTRL=$sc" || true
    # The verdict must fail for the intended reason, never route (2/3) and
    # never "control failed to fail" (4).
    if grep -q "exit=4" "$OUT/ctrl_$sc.log" 2>/dev/null; then
        echo "FAIL ctrl_$sc: verdict unexpectedly passed (exit 4 path)"
        overall=1
    fi
done

# --- positive control: counter hit reporting --------------------------------
run_harness ctrl_counter_hit "$HARNESS/w18b_controls.gdb" 0 \
    "XENO_W18I_CTRL=counter_hit" || true
grep -q "label=74e58 status=HIT registered=1 hits=1" \
    "$OUT/ctrl_counter_hit.log"
if [ $? -ne 0 ]; then
    echo "FAIL ctrl_counter_hit: 74e58 did not report HIT hits=1"
    overall=1
fi

echo
if [ "$overall" -eq 0 ]; then
    echo "W18I SUITE: PASS"
else
    echo "W18I SUITE: FAIL"
fi
exit "$overall"
