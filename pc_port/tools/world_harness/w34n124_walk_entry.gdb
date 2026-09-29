# W34N124 natural on-foot world walk + location entry harness.
#
# Route: accepted natural Lahan exit-1 route (XENO_FIELD_TEST=1,
# XENO_KERNEL_SEL=0, XENO_FIELD_MAP=1, XENO_FIELD_ENTRANCE=0, field schedule
# 0x2000 <600, 0x4000 600..916, 0 after).  Once the world session is live the
# world-frame-relative schedule XENO_WORLD_TEST_INPUT drives slot 1
# (wm_8008A72C, the on-foot player): d-pad nibble 0x2000 walks Fei from the
# arrival point across the Lahan Village trigger (list-0 record 1, map 1,
# entrance 9); a single 0x0020 (circle) rising edge on that trigger drives
# wm_80090A84 class 1 -> D554=0 -> terminal-zero lane -> FieldMain map 1.
#
# The breakpoint on wm_8008A72C prints W34N124 lines (guest position, heading,
# trigger selection) every 30 calls; nothing is written to guest memory.
#
# Invocation (repo root, host display :10):
#   DISPLAY=:10 timeout -s INT 420 gdb -batch \
#     -x pc_port/tools/world_harness/w34n124_walk_entry.gdb \
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
set environment XENO_WORLD_FRAME_LIMIT 400
set environment XENO_TEST_INPUT 0:0x2000,600:0x4000,916:0x2000,976:0
# XENO_WORLD_TEST_INPUT is inherited from the invoking shell (see
# pc_port/tests/run_w34n124_world_walk_entry.sh for the per-target schedules).
set environment SDL_AUDIODRIVER dummy

python
import gdb, struct

def rd(addr, n):
    off = addr & 0x1fffff
    base = int(gdb.parse_and_eval("(unsigned long)&g_PsxRam[0]"))
    return bytes(gdb.selected_inferior().read_memory(base + off, n))
def u32(a): return struct.unpack('<I', rd(a,4))[0]
def s32(a): return struct.unpack('<i', rd(a,4))[0]
def s16(a): return struct.unpack('<h', rd(a,2))[0]
def u16(a): return struct.unpack('<H', rd(a,2))[0]
def native_rd(addr, n):
    return bytes(gdb.selected_inferior().read_memory(addr, n))
def native_u32(a): return struct.unpack('<I', native_rd(a,4))[0]
def native_s16(a): return struct.unpack('<h', native_rd(a,2))[0]
def native_s8(a): return struct.unpack('<b', native_rd(a,1))[0]

class PaceBreakpoint(gdb.Breakpoint):
    def stop(self):
        return False

class S1Breakpoint(gdb.Breakpoint):
    def __init__(self, *a, **k):
        super().__init__(*a, **k)
        self.n = 0
    def stop(self):
        self.n += 1
        pool = u32(0x8009BE24)
        idx = int(gdb.parse_and_eval("$rdi"))
        slot = pool + (idx << 7)
        if self.n == 1:
            tab = u32(0x8009BD00)
            lst = u32(tab)
            rec = lst
            i = 0
            while s16(rec + 8) != -1 and i < 64:
                print("W34N124 list0 rec[%d] x0=%d z0=%d w=%d h=%d map=%d entrance=%d id=%d type=%d" % (
                    i, s16(rec), s16(rec+2), s16(rec+4), s16(rec+6), u16(rec+8), u16(rec+0xA), u16(rec+0xC), s16(rec+0xE)))
                rec += 0x10
                i += 1
        if self.n % 30 == 1 or self.n < 3:
            obj = u32(slot+0x4c)
            print("W34N124 call=%d slot=%d state=%d pos=(%d,%d,%d) head=0x%03x cd4c=0x%04x bd10=0x%04x BD24=%d D7D8=0x%08x D554=%d obj=0x%08x anim=%d pose=%d wait=%d script=0x%08x" % (
                self.n, idx, s16(slot+0x20), s32(slot+0x28)>>12, s32(slot+0x2c)>>12, s32(slot+0x30)>>12,
                u16(slot+0x48), u16(0x8009CD4C), u16(0x8009BD10), s16(0x8009BD24), u32(0x8009D7D8), u32(0x8009D554),
                obj, native_s8(obj+0xaf), native_s16(obj+0x34), native_s16(obj+0x9e), native_u32(obj+0x64)))
        return False

PaceBreakpoint("func_8007554C", internal=True)
PaceBreakpoint("func_8009F5F4", internal=True)
S1Breakpoint("wm_8008A72C", internal=True)
end

run
bt 12
quit 0
