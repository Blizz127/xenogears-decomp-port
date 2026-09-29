# W18I fault-injection controls harness.
#
# Runs the accepted W18B natural route with one injected instrumentation
# fault selected by XENO_W18I_CTRL. Every fault scenario must FAIL (gdb exit
# nonzero) for the intended reason; the two counter-hit scenarios prove the
# native route-counter mechanism reports real hits.
#
# Scenarios (XENO_W18I_CTRL):
#   unregistered_required  required target declared with no method
#                          -> NOT_INSTRUMENTED, exit nonzero
#   bad_symbol             required breakpoint on nonexistent symbol
#                          -> registration failure recorded, exit nonzero
#   missing_counter        required counter whose symbol is absent
#                          -> NOT_INSTRUMENTED (not zero), exit nonzero
#   disabled_bp            required breakpoint disabled before the interval
#                          -> NOT_INSTRUMENTED, exit nonzero
#   deleted_bp             required breakpoint deleted during the interval
#                          -> INSTRUMENTATION_ERROR, exit nonzero
#   hit_required           the ported 0x80074E58 rung is dispatched under
#                          harness control: first call runs the real body,
#                          the blocked second call trips the residual counter
#                          while 74e58 is required-zero -> HIT, exit nonzero
#   counter_hit            same double dispatch, 74e58 declared optional with
#                          expect hit_exact:1 -> HIT count=1, exit 0
#
# Invocation (repo root, host display :10), e.g.:
#   DISPLAY=:10 XENO_W18I_CTRL=bad_symbol timeout -s KILL 300 gdb -batch \
#     -x pc_port/tools/world_harness/w18b_controls.gdb \
#     --args pc_port/build_native/xeno-port
set pagination off
set confirm off
set debuginfod enabled off
set breakpoint pending off
set print thread-events off
set environment XENO_FIELD_TEST 1
set environment XENO_KERNEL_SEL 0
set environment XENO_FIELD_MAP 1
set environment XENO_FIELD_ENTRANCE 0
set environment XENO_WORLD_HEAP_TABLE_RAND 1
unset environment XENO_WORLD_FT4_POOLS
set environment SDL_AUDIODRIVER dummy
set environment LD_LIBRARY_PATH /home/blizz/dev/xenogears-assets/lib

python
import os
import sys

import gdb

_repo = os.environ.get("XENO_REPO_ROOT") or os.getcwd()
sys.path.insert(0, os.path.join(_repo, "pc_port", "tools", "world_harness"))
import forbidden_instrumentation as fi

SCENARIO = os.environ.get("XENO_W18I_CTRL", "")
FAULT_SCENARIOS = ("unregistered_required", "bad_symbol", "missing_counter",
                   "disabled_bp", "deleted_bp", "hit_required")

frame = 0
state3 = False
in_placeholder = False
saw_htr = False
placeholder_vsyncs = 0

instr = fi.ForbiddenInstrumentation("W18ICTRL")
print("W18ICTRL_SCENARIO scenario=%s" % (SCENARIO or "none"))


def as_int(expr):
    value = gdb.parse_and_eval(expr)
    if value.type.code == gdb.TYPE_CODE_PTR:
        value = value.cast(gdb.lookup_type("uintptr_t"))
    return int(value)


def finish(route_failure=0):
    """route_failure: nonzero when the route itself broke (quit that code
    regardless of verdict). Otherwise the verdict decides; a fault scenario
    whose verdict unexpectedly passes exits 4 (control failed to fail)."""
    instr.end_interval()
    instr.emit_summary()
    code, _ = instr.verdict()
    if route_failure:
        code = route_failure
    elif SCENARIO in FAULT_SCENARIOS and code == 0:
        code = 4
    gdb.execute("quit %d" % code)


class FrameBreakpoint(gdb.Breakpoint):
    def stop(self):
        global frame
        frame += 1
        if frame == 1:
            instr.start_interval()
        if frame > 2500 and not in_placeholder:
            print("W18ICTRL_FAIL timeout")
            finish(route_failure=2)
        return False


class InputBreakpoint(gdb.Breakpoint):
    def stop(self):
        if frame < 600:
            d = 0x2000
        elif frame <= 916:
            d = 0x4000
        else:
            d = 0
        gdb.execute("set *(unsigned short*)&D_800AFE9C = %d" % d,
                    to_string=True)
        return False


class ChangeStateBreakpoint(gdb.Breakpoint):
    def stop(self):
        global state3
        if as_int("$rdi") == 3:
            state3 = True
        return False


class ForbidSymbolBreakpoint(gdb.Breakpoint):
    def __init__(self, target):
        super().__init__(target.symbol, internal=True)
        self.target = target

    def stop(self):
        global saw_htr
        if self.target.label == "863e0_dispatch":
            saw_htr = True
        instr.record_hit(self.target.label)
        return False


class AttributionBreakpoint(gdb.Breakpoint):
    def __init__(self, target):
        super().__init__(target.symbol, internal=True)
        self.target = target

    def stop(self):
        try:
            world = fi.backtrace_is_world_residual()
        except Exception:
            self.target.lost_reason = "attribution_error"
            return False
        instr.record_hit(self.target.label, allowed=not world)
        return False


class PlaceholderBreakpoint(gdb.Breakpoint):
    def stop(self):
        global in_placeholder
        if in_placeholder:
            return False
        in_placeholder = True
        print("W18ICTRL_PLACEHOLDER state3=%d htr=%d scenario=%s" %
              (int(state3), int(saw_htr), SCENARIO or "none"))
        if not (state3 and saw_htr):
            print("W18ICTRL_FAIL route preconditions not met")
            finish(route_failure=2)
            return False

        if SCENARIO == "deleted_bp":
            # Delete the control breakpoint during the measured interval.
            t = instr.by_label["ctrl_deleted"]
            try:
                t.bp.delete()
                print("W18ICTRL_FAULT ctrl_deleted breakpoint deleted "
                      "mid-interval")
            except Exception as exc:
                print("W18ICTRL_FAULT_ERR %s" % exc)
            finish()
            return False

        if SCENARIO in ("hit_required", "counter_hit"):
            # Halt the inferior here; the wrapper dispatch runs as a
            # top-level inferior call after `run` returns (an inferior call
            # nested inside a breakpoint stop callback deadlocks gdb).
            print("W18ICTRL_HOLD_AT_PLACEHOLDER for controlled dispatch")
            return True

        if SCENARIO in FAULT_SCENARIOS:
            # Fault verdict is decidable as soon as the interval ran; skip
            # the 120-vsync stability tail.
            finish()
        return False


class VsyncBreakpoint(gdb.Breakpoint):
    def stop(self):
        global placeholder_vsyncs
        if in_placeholder:
            placeholder_vsyncs += 1
            instr.check_interval_health()
            if placeholder_vsyncs == 120:
                print("W18ICTRL_DONE stable=120")
                instr.end_interval()
                instr.emit_summary()
                code, _ = instr.verdict()
                gdb.execute("quit %d" % code)
        return False


# ----------------------- strict forbidden manifest ------------------------
instr.symbol_target("712d0", "wm_800712D0_should_not_run",
                    bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("72238", "wm_80072238_should_not_run",
                    bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("7299c", "wm_8007299C_should_not_run",
                    bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("9766c", "wm_8009766C_should_not_run",
                    bp_factory=ForbidSymbolBreakpoint)

if SCENARIO == "hit_required":
    # Required-zero counter that the scenario deliberately increments.
    instr.counter_target("74e58")
elif SCENARIO == "counter_hit":
    # Optional positive control: must report HIT count=1.
    instr.counter_target("74e58", required=False, expect="hit_exact:1")
else:
    instr.counter_target("74e58")
instr.counter_target("75030")
instr.counter_target("739b8")
instr.counter_target("88f64")
instr.counter_target("37fd8")
instr.counter_target("drawotag_world")
instr.counter_target("cdsync_world")

instr.attribution_target("cdsync_attr", "ArchiveCdDataSync",
                         bp_factory=AttributionBreakpoint)
instr.attribution_target("drawotag_attr", "DrawOTag",
                         bp_factory=AttributionBreakpoint)

instr.symbol_target("863e0_dispatch", "wm_800863E0_init_heap_table_rand",
                    required=False, expect="hit_exact:1",
                    bp_factory=ForbidSymbolBreakpoint)

# ----------------------- scenario fault injection -------------------------
if SCENARIO == "unregistered_required":
    instr.uninstrumented_target(
        "ctrl_unregistered", "declared_without_registration_method")
elif SCENARIO == "bad_symbol":
    instr.symbol_target("ctrl_bad_symbol",
                        "wm_ctrl_symbol_does_not_exist_18i")
elif SCENARIO == "missing_counter":
    instr.counter_target("ctrl_missing_counter",
                         counter_expr="w18i_ctrl_counter_missing")
elif SCENARIO == "disabled_bp":
    instr.symbol_target("ctrl_disabled", "wm_800712D0_should_not_run")
elif SCENARIO == "deleted_bp":
    instr.symbol_target("ctrl_deleted", "wm_80072238_should_not_run")

FrameBreakpoint("func_8007554C", internal=True)
InputBreakpoint("func_8009F5F4", internal=True)
ChangeStateBreakpoint("ChangeGameState", internal=True)
PlaceholderBreakpoint("PcPort_WorldMapPlaceholderMain", internal=True)
VsyncBreakpoint("Vsync", internal=True)

instr.register_all()

if SCENARIO == "disabled_bp":
    _t = instr.by_label["ctrl_disabled"]
    if _t.bp is not None:
        _t.bp.enabled = False
        print("W18ICTRL_FAULT ctrl_disabled breakpoint disabled "
              "before interval start")
end

run

python
# Post-run (hit scenarios only): the inferior is stopped at the placeholder
# breakpoint, so the controlled dispatches are plain top-level inferior calls.
# The route ran W18B but not W19A (no XENO_WORLD_UPLOAD_RECORDS here), so the
# first call executes the real W19A body once and the second call is blocked
# by the one-shot guard, tripping the residual counter s_wm74e58_hits.
if SCENARIO in ("hit_required", "counter_hit"):
    try:
        gdb.execute("print wm_80074E58_build_upload_records()", to_string=True)
        gdb.execute("print wm_80074E58_build_upload_records()", to_string=True)
        print("W18ICTRL_FAULT 74e58 rung dispatched twice "
              "(first run, second blocked)")
    except Exception as exc:
        print("W18ICTRL_FAULT_ERR inferior call failed: %s" % exc)
        gdb.execute("quit 3")
    instr.end_interval()
    instr.emit_summary()
    code, _ = instr.verdict()
    if SCENARIO == "counter_hit":
        gdb.execute("quit %d" % code)
    gdb.execute("quit %d" % (code if code else 4))
end
# Reached only if the inferior died without the harness quitting first.
quit 5
