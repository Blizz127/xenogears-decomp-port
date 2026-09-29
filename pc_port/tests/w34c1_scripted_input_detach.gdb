set pagination off
set confirm off
set debuginfod enabled off
set breakpoint pending off
set print thread-events off

set environment XENO_FIELD_TEST 1
set environment XENO_KERNEL_SEL 0
set environment XENO_FIELD_MAP 1
set environment XENO_FIELD_ENTRANCE 0
set environment XENO_WORLD_ARCHIVE_SET_INDEX 1
set environment XENO_WORLD_967E4_ROUTE 1
set environment XENO_WORLD_READY_BUFFER_CONSUME 1
set environment XENO_WORLD_MODE_AUDIO_SETUP 1
set environment XENO_WORLD_CONVERGENCE_P1 1
set environment XENO_WORLD_CONVERGENCE_P2 1
set environment XENO_WORLD_FRAMEBUFFER_GTE_INIT 1
set environment XENO_WORLD_TERRAIN_POSITION_INIT 1
set environment XENO_WORLD_COMMON_TAIL_P0 1
set environment XENO_WORLD_COMMON_TAIL_P1 1
set environment XENO_WORLD_COMMON_TAIL_P2 1
set environment XENO_WORLD_COMMON_TAIL_P3 1
set environment XENO_WORLD_COMMON_TAIL_P4 1
set environment XENO_WORLD_COMMON_TAIL_P5 1
set environment XENO_WORLD_SCHEDULER_97800 1
set environment XENO_WORLD_FRAME_PROLOGUE 1
set environment XENO_WORLD_DRAW_PACKETS 1
set environment XENO_WORLD_OPEN_LOOP 1
set environment XENO_WORLD_FRAME_LIMIT 120
set environment SDL_AUDIODRIVER dummy
set environment LD_LIBRARY_PATH /home/blizz/dev/xenogears-assets/lib

python
import gdb

class DetachBreakpoint(gdb.Breakpoint):
    def stop(self):
        print("W34C1_SCRIPTED_INPUT_DETACH pid=%d" %
              gdb.selected_inferior().pid)
        gdb.execute("detach", to_string=True)
        gdb.execute("quit 0")
        return False

DetachBreakpoint("PcPort_WorldMapInitMain", internal=True)
end

run
