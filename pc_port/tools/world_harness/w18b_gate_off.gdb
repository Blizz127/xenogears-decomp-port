# W18I hardened gate-off smoke: XENO_WORLD_FT4_POOLS=1 without
# XENO_WORLD_HEAP_TABLE_RAND. The ladder must stop at the W17B cut
# (retail 0x80072488): W17B FT4 pools run, W18B never dispatches, and every
# required world forbidden target stays ZERO VERIFIED under full
# instrumentation.
#
# Invocation (repo root, host display :10, never Docker):
#   DISPLAY=:10 timeout -s KILL 300 gdb -batch \
#     -x pc_port/tools/world_harness/w18b_gate_off.gdb \
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
set environment XENO_WORLD_FT4_POOLS 1
unset environment XENO_WORLD_HEAP_TABLE_RAND
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
saw_ft4 = False
saw_htr = False
placeholder_vsyncs = 0
w18b_entries = 0

instr = fi.ForbiddenInstrumentation("W18IGATE")


def as_int(expr):
    value = gdb.parse_and_eval(expr)
    if value.type.code == gdb.TYPE_CODE_PTR:
        value = value.cast(gdb.lookup_type("uintptr_t"))
    return int(value)


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
            instr.start_interval()
        if frame > 2500 and not in_placeholder:
            print("W18IGATE_FAIL timeout")
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


class ChangeStateBreakpoint(gdb.Breakpoint):
    def stop(self):
        global state3
        if as_int("$rdi") == 3:
            state3 = True
            print("W18IGATE_STATE3")
        return False


class Ft4Breakpoint(gdb.Breakpoint):
    def stop(self):
        global saw_ft4
        saw_ft4 = True
        print("W18IGATE_FT4_HIT")
        return False


class ForbidSymbolBreakpoint(gdb.Breakpoint):
    def __init__(self, target):
        super().__init__(target.symbol, internal=True)
        self.target = target

    def stop(self):
        global saw_htr, w18b_entries
        if self.target.label == "863e0_dispatch":
            saw_htr = True
            w18b_entries += 1
            print("W18IGATE_HTR_HIT unexpected")
        instr.record_hit(self.target.label)
        print("W18IGATE_FORBID_HIT label=%s count=%s" %
              (self.target.label, instr.by_label[self.target.label].hit_count))
        return False


class AttributionBreakpoint(gdb.Breakpoint):
    def __init__(self, target):
        super().__init__(target.symbol, internal=True)
        self.target = target

    def stop(self):
        try:
            world = fi.backtrace_is_world_residual()
        except Exception as exc:
            print("W18IGATE_ATTRIB_ERROR label=%s err=%s" %
                  (self.target.label, exc))
            self.target.lost_reason = "attribution_error"
            return False
        instr.record_hit(self.target.label, allowed=not world)
        if world:
            print("W18IGATE_FORBID_HIT label=%s count=%s (world residual)" %
                  (self.target.label,
                   instr.by_label[self.target.label].hit_count))
        return False


class PlaceholderBreakpoint(gdb.Breakpoint):
    def stop(self):
        global in_placeholder
        in_placeholder = True
        print("W18IGATE_PLACEHOLDER state3=%d ft4=%d htr=%d" %
              (int(state3), int(saw_ft4), int(saw_htr)))
        if not (state3 and saw_ft4 and not saw_htr):
            print("W18IGATE_FAIL precondition: state3=1 ft4=1 htr=0")
            finish(2)
        return False


class VsyncBreakpoint(gdb.Breakpoint):
    def stop(self):
        global placeholder_vsyncs
        if in_placeholder:
            placeholder_vsyncs += 1
            instr.check_interval_health()
            if placeholder_vsyncs == 120:
                print("W18IGATE_DONE stable=120 ft4=1 htr=0")
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
                    expect="zero", bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("74e58_dispatch", "wm_80074E58_build_upload_records",
                    expect="zero", bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("75030_dispatch", "wm_80075030_build_upload_records_b",
                    expect="zero", bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("739b8_dispatch", "wm_800739B8_build_draw_packets",
                    expect="zero", bp_factory=ForbidSymbolBreakpoint)

FrameBreakpoint("func_8007554C", internal=True)
InputBreakpoint("func_8009F5F4", internal=True)
ChangeStateBreakpoint("ChangeGameState", internal=True)
Ft4Breakpoint("wm_80074594_init_ft4_pools", internal=True)
PlaceholderBreakpoint("PcPort_WorldMapPlaceholderMain", internal=True)
VsyncBreakpoint("Vsync", internal=True)

instr.register_all()
end

run
