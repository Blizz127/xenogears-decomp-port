# W18I hardened W18B hold-enabled harness (deepest gate).
#
# Same input route as the natural harness but with
# XENO_FIELD_HOLD_TRANSITION=1 and the deepest world gate
# XENO_WORLD_DRAW_PACKETS=1 (implies W20B/W19A/W18B): the held-field
# transition vetoes world entry, so state3 is never reached and no world
# rung executes. Every required world forbidden target must still be
# instrumented and end ZERO VERIFIED; field calls of shared functions
# (DrawOTag/ArchiveCdDataSync) are classified allowed by backtrace
# attribution and never counted against world targets.
#
# Invocation (repo root, host display :10, never Docker):
#   DISPLAY=:10 timeout -s KILL 300 gdb -batch \
#     -x pc_port/tools/world_harness/w18b_hold_enabled.gdb \
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
set environment XENO_WORLD_DRAW_PACKETS 1
set environment XENO_FIELD_HOLD_TRANSITION 1
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

HOLD_FRAMES = 1400

frame = 0
armed = False
state3 = False
in_w18b = False
w18b_entries = 0

instr = fi.ForbiddenInstrumentation("W18IHOLD")


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


class FrameBreakpoint(gdb.Breakpoint):
    def stop(self):
        global frame
        frame += 1
        if frame == 1:
            instr.start_interval()
        if frame in (814, 916):
            print("W18IHOLD_FRAME frame=%d tuple=%s" % (frame, tuple_values()))
        if frame >= HOLD_FRAMES:
            print("W18IHOLD_REACHED frame=%d armed=%d state3=%d w18b=%d" %
                  (frame, int(armed), int(state3), w18b_entries))
            if state3 or w18b_entries:
                print("W18IHOLD_FAIL world entered despite hold")
                instr.end_interval()
                instr.emit_summary()
                instr.verdict()
                gdb.execute("quit 2")
            if not armed:
                print("W18IHOLD_FAIL route never armed")
                gdb.execute("quit 2")
            instr.end_interval()
            instr.emit_summary()
            code, _ = instr.verdict()
            gdb.execute("quit %d" % code)
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
        global armed
        armed = True
        print("W18IHOLD_ARM frame=%d tuple=%s" % (frame, tuple_values()))
        return False


class ChangeStateBreakpoint(gdb.Breakpoint):
    def stop(self):
        global state3
        if as_int("$rdi") == 3:
            state3 = True
            instr.record_hit("state3_entries")
            print("W18IHOLD_STATE3 unexpected")
        return False


class ForbidSymbolBreakpoint(gdb.Breakpoint):
    def __init__(self, target):
        super().__init__(target.symbol, internal=True)
        self.target = target

    def stop(self):
        global in_w18b
        if self.target.label == "863e0_dispatch":
            in_w18b = True
        instr.record_hit(self.target.label)
        print("W18IHOLD_FORBID_HIT label=%s count=%s" %
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
            print("W18IHOLD_ATTRIB_ERROR label=%s err=%s" %
                  (self.target.label, exc))
            self.target.lost_reason = "attribution_error"
            return False
        instr.record_hit(self.target.label, allowed=not world)
        if world:
            print("W18IHOLD_FORBID_HIT label=%s count=%s (world residual)" %
                  (self.target.label,
                   instr.by_label[self.target.label].hit_count))
        return False


class RandBreakpoint(gdb.Breakpoint):
    def stop(self):
        if in_w18b:
            instr.record_hit("rand_in_w18b")
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

# Hold-route specific forbidden targets: W18B dispatch, W19A dispatch, W20B
# dispatch, W21B dispatch and world RNG stay zero; world state is never
# entered.
instr.symbol_target("863e0_dispatch", "wm_800863E0_init_heap_table_rand",
                    expect="zero", bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("74e58_dispatch", "wm_80074E58_build_upload_records",
                    expect="zero", bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("75030_dispatch", "wm_80075030_build_upload_records_b",
                    expect="zero", bp_factory=ForbidSymbolBreakpoint)
instr.symbol_target("739b8_dispatch", "wm_800739B8_build_draw_packets",
                    expect="zero", bp_factory=ForbidSymbolBreakpoint)
instr.window_target("rand_in_w18b")
instr.window_target("state3_entries")

FrameBreakpoint("func_8007554C", internal=True)
InputBreakpoint("func_8009F5F4", internal=True)
ArmBreakpoint("PcPort_FieldOpcode56TransitionIntercept", internal=True)
ChangeStateBreakpoint("ChangeGameState", internal=True)
RandBreakpoint("rand", internal=True)

instr.register_all()
end

run
