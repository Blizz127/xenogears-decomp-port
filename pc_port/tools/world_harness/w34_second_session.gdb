# Second world-map session in one visit (regression for the exit() crash).
#
# Route: the W34N124 Lahan on-foot route (see w34n124_walk_entry.gdb).  When
# the first session ends (terminal lane, world overlay still resident) the
# harness runs retail's per-session setup 0x80072238 again, as a session
# change inside one visit does.  Before the fix the object-pool stage refused
# the still-set pool pointer (and the other stages their one-shot guards), so
# the setup failed and wm_80071034 called exit().  Pass: the setup returns 0.
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
set environment SDL_AUDIODRIVER dummy

break wm_71034_run_terminal_zero_lane
commands
  silent
  printf "WM2 first session ended; pool=0x%08x\n", *(unsigned int*)&g_PsxRam[0x9BE24]
  printf "WM2 second-session setup rc=%d\n", (int)wm_80072238()
  printf "WM2 pool after=0x%08x\n", *(unsigned int*)&g_PsxRam[0x9BE24]
  quit 0
end
run
printf "WM2 terminal lane never reached\n"
quit 1
