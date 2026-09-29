# W18I hardened natural Lahan harness, extended by the W19A + W20B + W21B rungs.
#
# Route is the accepted W18B natural route with the deepest gate advanced to
# XENO_WORLD_DRAW_PACKETS=1 (implies XENO_WORLD_UPLOAD_RECORDS_B,
# XENO_WORLD_UPLOAD_RECORDS and XENO_WORLD_HEAP_TABLE_RAND):
# XENO_FIELD_TEST=1, XENO_KERNEL_SEL=0,
# XENO_FIELD_MAP=1, XENO_FIELD_ENTRANCE=0, FT4_POOLS unset, hold unset;
# input D_800AFE9C = 0x2000 (frame<600), 0x4000 (600..916), 0 afterwards;
# cut at retail 0x800724A8 (before jal 0x80088F64); placeholder stable 120
# Vsyncs.
#
# Hardening: every required forbidden target has proven registration and the
# run ends in the strict machine-readable summary. A target is ZERO VERIFIED
# only when its instrumentation was registered and stayed active for the whole
# measured interval (process start -> 120th placeholder Vsync). Unregistered
# targets are NOT INSTRUMENTED, never numeric zero. The ported 0x80074E58 /
# 0x80075030 / 0x800739B8 rungs (W19A/W20B/W21B) are hit_exact:1 positive
# controls; their registry counters now count blocked residual re-dispatches
# and must stay zero.
#
# Invocation (repo root, host display :10, never Docker):
#   DISPLAY=:10 timeout -s KILL 300 gdb -batch \
#     -x pc_port/tools/world_harness/w18b_natural.gdb \
#     --args pc_port/build_native/xeno-port
set pagination off
set confirm off
set debuginfod enabled off
# Pending breakpoints are never accepted as registration proof.
set breakpoint pending off
set print thread-events off
set environment XENO_FIELD_TEST 1
set environment XENO_KERNEL_SEL 0
set environment XENO_FIELD_MAP 1
set environment XENO_FIELD_ENTRANCE 0
set environment XENO_WORLD_DRAW_PACKETS 1
unset environment XENO_WORLD_UPLOAD_RECORDS_B
unset environment XENO_WORLD_UPLOAD_RECORDS
unset environment XENO_WORLD_HEAP_TABLE_RAND
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

frame = 0
state3 = False
in_placeholder = False
saw_gfx = False
saw_ft4 = False
saw_htr = False
rand_seen = False
placeholder_vsyncs = 0
w18b_entries = 0

instr = fi.ForbiddenInstrumentation("W18I")


def as_int(expr):
    value = gdb.parse_and_eval(expr)
    if value.type.code == gdb.TYPE_CODE_PTR:
        value = value.cast(gdb.lookup_type("uintptr_t"))
    return int(value)


def read_bytes(addr, count):
    return bytes(gdb.selected_inferior().read_memory(addr, count))


def pointer_slot(name):
    return int.from_bytes(read_bytes(as_int("&" + name), 8), "little")


def tuple_values():
    game = pointer_slot("g_pGameState")
    return tuple(int.from_bytes(read_bytes(game + off, 2), "little")
                 for off in (0x231a, 0x231c, 0x231e, 0x2320))


def finish(exit_code):
    instr.end_interval()
    instr.emit_summary()
    instr.verdict()
    gdb.execute("quit %d" % exit_code)


class FrameBreakpoint(gdb.Breakpoint):
    def stop(self):
        global frame
        frame += 1
        if frame == 1:
            # Earliest live point: complete registration proof and open the
            # measured interval. All instrumentation must already exist.
            instr.start_interval()
        if frame in (814, 916):
            print("W18I_FRAME frame=%d tuple=%s" % (frame, tuple_values()))
        if frame > 2500 and not in_placeholder:
            print("W18I_FAIL timeout")
            finish(2)
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


class ArmBreakpoint(gdb.Breakpoint):
    def stop(self):
        print("W18I_ARM frame=%d tuple=%s" % (frame, tuple_values()))
        return False


class ChangeStateBreakpoint(gdb.Breakpoint):
    def stop(self):
        global state3
        if as_int("$rdi") == 3:
            state3 = True
            print("W18I_STATE3")
        return False


class GfxBreakpoint(gdb.Breakpoint):
    def stop(self):
        global saw_gfx
        saw_gfx = True
        print("W18I_GFX_WORK_HIT")
        return False


class Ft4Breakpoint(gdb.Breakpoint):
    def stop(self):
        global saw_ft4
        saw_ft4 = True
        print("W18I_FT4_POOLS_HIT")
        return False


class RandBreakpoint(gdb.Breakpoint):
    def stop(self):
        global rand_seen
        if not rand_seen:
            rand_seen = True
            print("W18I_RAND_HIT")
        return False


class HeapTableRandBreakpoint(gdb.Breakpoint):
    def __init__(self, target):
        super().__init__(target.symbol, internal=True)
        self.target = target

    def stop(self):
        global saw_htr, w18b_entries
        saw_htr = True
        w18b_entries += 1
        instr.record_hit(self.target.label)
        print("W18I_HEAP_TABLE_RAND_HIT entry=%d" % w18b_entries)
        return False


class ForbidSymbolBreakpoint(gdb.Breakpoint):
    """Counts hits for a registered symbol target."""

    def __init__(self, target):
        super().__init__(target.symbol, internal=True)
        self.target = target

    def stop(self):
        instr.record_hit(self.target.label)
        print("W18I_FORBID_HIT label=%s count=%s" %
              (self.target.label, instr.by_label[self.target.label].hit_count))
        return False


class AttributionBreakpoint(gdb.Breakpoint):
    """Shared-function breakpoint with backtrace caller classification."""

    def __init__(self, target):
        super().__init__(target.symbol, internal=True)
        self.target = target

    def stop(self):
        try:
            world = fi.backtrace_is_world_residual()
        except Exception as exc:
            print("W18I_ATTRIB_ERROR label=%s err=%s" %
                  (self.target.label, exc))
            self.target.lost_reason = "attribution_error"
            return False
        instr.record_hit(self.target.label, allowed=not world)
        if world:
            print("W18I_FORBID_HIT label=%s count=%s (world residual)" %
                  (self.target.label,
                   instr.by_label[self.target.label].hit_count))
        return False


class CountingBreakpoint(gdb.Breakpoint):
    def __init__(self, target):
        super().__init__(target.symbol, internal=True)
        self.target = target

    def stop(self):
        instr.record_hit(self.target.label)
        return False


class PlaceholderBreakpoint(gdb.Breakpoint):
    def stop(self):
        global in_placeholder
        in_placeholder = True
        print("W18I_PLACEHOLDER state3=%d gfx=%d ft4=%d htr=%d rand=%d "
              "w18b_entries=%d" %
              (int(state3), int(saw_gfx), int(saw_ft4), int(saw_htr),
               int(rand_seen), w18b_entries))
        if not (state3 and saw_gfx and saw_ft4 and saw_htr and rand_seen):
            print("W18I_PLACEHOLDER_FAIL route preconditions not met")
            finish(2)
        return False


class VsyncBreakpoint(gdb.Breakpoint):
    def stop(self):
        global placeholder_vsyncs
        if in_placeholder:
            placeholder_vsyncs += 1
            instr.check_interval_health()
            if placeholder_vsyncs == 120:
                print("W18I_DONE stable=120 ft4=1 htr=1 rand=1")
                instr.end_interval()
                instr.emit_summary()
                code, _ = instr.verdict()
                gdb.execute("quit %d" % code)
        return False


# ----------------------- strict forbidden manifest ------------------------
# Required forbidden targets: existing port stubs occupying the retail steps'
# native dispatch slots (W12B-W18B convention).
instr.symbol_target("712d0", "wm_800712D0_should_not_run",
                    bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("72238", "wm_80072238_should_not_run",
                    bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("7299c", "wm_8007299C_should_not_run",
                    bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("9766c", "wm_8009766C_should_not_run",
                    bp_factory=ForbidSymbolBreakpoint)

# Required forbidden targets: native route-boundary counters for retail steps
# with no native code (absent from the linked image). Counters live in the
# exported g_wm_forbidden_targets registry in world_map_init.c. The 74e58 and
# 75030 counters survive the W19A/W20B ports as residual re-dispatch
# counters: the ported rungs only increment them when a blocked second
# dispatch is attempted.
instr.counter_target("74e58")
instr.counter_target("75030")
instr.counter_target("739b8")
instr.counter_target("88f64")
instr.counter_target("37fd8")
instr.counter_target("drawotag_world")
instr.counter_target("cdsync_world")

# Required forbidden targets: shared-function attribution breakpoints. Live
# catch-alls: any world-route residual call of these shared functions is
# classified by backtrace; field/overlay/placeholder callers are allowed and
# reported separately (W18E3R attribution: 29 legitimate ArchiveCdDataSync
# overlay-load hits after state3, 119 placeholder DrawOTag hits).
instr.attribution_target("cdsync_attr", "ArchiveCdDataSync",
                         bp_factory=AttributionBreakpoint)
instr.attribution_target("drawotag_attr", "DrawOTag",
                         bp_factory=AttributionBreakpoint)

# Positive controls: known-hit targets — W18B executes exactly once, and the
# ported W19A (0x80074E58), W20B (0x80075030) and W21B (0x800739B8) rungs each
# execute exactly once on this route — plus multi-hit
# (Vsync fires at least 120 times). Positive controls are not forbidden
# targets, so they are declared optional; the verdict still enforces the
# expectation (a mismatch classifies as INSTRUMENTATION_ERROR and fails the
# run).
instr.symbol_target("863e0_dispatch", "wm_800863E0_init_heap_table_rand",
                    required=False, expect="hit_exact:1",
                    bp_factory=HeapTableRandBreakpoint)
instr.symbol_target("74e58_dispatch", "wm_80074E58_build_upload_records",
                    required=False, expect="hit_exact:1",
                    bp_factory=CountingBreakpoint)
instr.symbol_target("75030_dispatch", "wm_80075030_build_upload_records_b",
                    required=False, expect="hit_exact:1",
                    bp_factory=CountingBreakpoint)
instr.symbol_target("739b8_dispatch", "wm_800739B8_build_draw_packets",
                    required=False, expect="hit_exact:1",
                    bp_factory=CountingBreakpoint)
instr.symbol_target("vsync_multihit", "Vsync", required=False,
                    expect="hit_min:120", bp_factory=CountingBreakpoint)

# Route breakpoints (accepted W18B natural route).
FrameBreakpoint("func_8007554C", internal=True)
InputBreakpoint("func_8009F5F4", internal=True)
ArmBreakpoint("PcPort_FieldOpcode56TransitionIntercept", internal=True)
ChangeStateBreakpoint("ChangeGameState", internal=True)
GfxBreakpoint("wm_route_gfx_allocate_work_buffers", internal=True)
Ft4Breakpoint("wm_80074594_init_ft4_pools", internal=True)
RandBreakpoint("rand", internal=True)
PlaceholderBreakpoint("PcPort_WorldMapPlaceholderMain", internal=True)
VsyncBreakpoint("Vsync", internal=True)

# Manifest registration proof (pre-run symbol resolution + registry lookup).
instr.register_all()
end

run
