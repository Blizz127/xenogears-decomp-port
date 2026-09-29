# VSync / present evidence (diagnostic, 2026-09-03)

## Finding

The visible field flicker is explained by an in-code double-present path; it is
not necessary to attribute the symptom to Linux. The port's `Vsync` wrapper
always tries to close/present a PsyCross scene, then additionally presents and
swaps the CPU VRAM display when `mode == 0` and the scene was already closed
(`pc_port/src/psyq_compat.c:1141-1169`). `PsyX_EndScene` performs the rendered
frame swap at `pc_port/extern/PsyCross/src/PsyX_main.cpp:883-900`, while the
VRAM fallback performs a second `GR_SwapWindow` at
`pc_port/extern/PsyCross/src/PsyX_main.cpp:871-880`.

The field frame order makes this reachable every frame:

1. `func_8007554C` calls `DrawOTag` at
   `src/field/main/misc2.c:2179`. PsyCross `DrawOTag` opens a scene, parses the
   list, and calls `DrawAllSplits`, but does not call `PsyX_EndScene`
   (`pc_port/extern/PsyCross/src/psx/LIBGPU.C:438-460`). The scene therefore
   remains open after the frame's final draw; the field code only does a
   non-blocking `Vsync(-1)` busy-wait afterward
   (`src/field/main/misc2.c:2181-2186`).
2. The next field iteration begins with `Vsync(1)`
   (`src/field/main/misc2.c:2020`). In the normal rendered path this closes the
   prior scene and swaps it through `PsyX_EndScene`.
3. The same iteration then reaches `Vsync(0)` after the render-context swap
   (`src/field/main/misc2.c:2066-2074`). At that point the prior scene is
   closed, so `PsyX_IsSceneOpen` is false and the wrapper invokes
   `PsyX_PresentDisplayFromVRAM`; that function blits the active DISPENV and
   calls `GR_SwapWindow` again (`pc_port/src/psyq_compat.c:1157-1169`,
   `pc_port/extern/PsyCross/src/PsyX_main.cpp:871-880`).

Thus a rendered frame can be followed by a second swap of the VRAM fallback
within the same field iteration. Depending on what is currently in the CPU
VRAM mirror, the second image can be stale/black/partial. This is a concrete
double presentation, sufficient to produce flicker; no compositor behavior is
needed to make the path incorrect.

## Pacing divergence

Retail `Vsync` treats positive modes as counted waits: `mode == 1` returns the
timing sample, while `mode > 0` computes a target and enters `v_wait`
(`src/slus_006.64/psyq/libetc/vsync.c:17-34`). PsyCross's `VSync` only waits
for `mode == 0`; the positive-mode branch is explicitly a FIXME/no-op
(`pc_port/extern/PsyCross/src/psx/LIBETC.C:31-48`). Therefore `Vsync(2)` does
not provide the retail wait/pacing contract. The world-map frame driver calls
`Vsync(2)` before its draw pipeline (`pc_port/src/world_map_frame_driver_712d0.c:453-501`),
and its bounded harness closes the scene manually after each draw
(`pc_port/src/world_map_main_loop_71034.c:181-194`). That path has no positive
mode wait, so it can run faster than retail and exhibit pacing divergence even
when presentation is otherwise visible.

## Input polling side effect (secondary)

The wrapper polls input once through `PsyX_UpdateInput`
(`pc_port/src/psyq_compat.c:1172-1177`; `pc_port/extern/PsyCross/src/PsyX_main.cpp:1061-1068`).
When the no-scene `mode == 0` fallback is taken, the presenter polls SDL events
again before the swap (`pc_port/extern/PsyCross/src/PsyX_main.cpp:871-879`).
`PsyX_BeginScene` also polls events (`pc_port/extern/PsyCross/src/PsyX_main.cpp:801-804`).
This is a real multi-drain behavior around a frame boundary, but it is not the
root cause of the visual flicker. The keyboard tap-latch patch exists to retain
KEYDOWN edges that an earlier poll could otherwise consume
(`pc_port/patches/psycross_keyboard_tap_latch.patch:8-13,25-39`).

## Linux/compositor boundary

PsyCross sets the OpenGL swap interval from `PsyX_BeginScene`
(`pc_port/extern/PsyCross/src/PsyX_main.cpp:801-832`), but the current default
`g_cfg_swapInterval` is zero (`pc_port/extern/PsyCross/src/PsyX_main.cpp:42-55`),
which selects interval 0. `GR_UpdateSwapIntervalState` forwards that to
`SDL_GL_SetSwapInterval` (`pc_port/extern/PsyCross/src/render/PsyX_render.cpp:501-506`).
Interval 0 permits compositor-visible tearing and can amplify the symptom, but
it cannot explain the extra swap itself; the double-present sequence above is
entirely in the port code.

## Focused regression-test shape (not implemented in this diagnostic)

Add a small C/C++ harness in the existing `pc_port/tests` style (for example,
`vsync_present_regression_test.cpp` plus a `run_*.sh` wrapper) with instrumented
fakes for `PsyX_IsSceneOpen`, `PsyX_EndScene`,
`PsyX_PresentDisplayFromVRAM`, `GR_SwapWindow`, `PsyX_Sys_DoPollEvent`, and
`VSync`/`PsyX_WaitForTimestep`:

- Rendered-frame sequence: mark a scene open as if `DrawOTag` just completed,
  call the next-frame `Vsync(1)`, then call the field's `Vsync(0)` boundary with
  the scene closed. Assert exactly one present/swap per completed rendered
  frame. The current wrapper should fail by recording the rendered
  `PsyX_EndScene` swap plus the VRAM-fallback swap.
- Empty-display sequence: call `Vsync(0)` with no scene and assert the fallback
  is allowed to present exactly once (this preserves movie/LoadImage behavior).
- Positive-mode pacing: call `Vsync(2)` and assert the underlying pacing fake is
  asked to wait for a positive/count-based interval. The current PsyCross
  implementation should fail because its `mode > 0` branch performs no wait.
- Poll accounting: record event-poll calls and assert a single per-vblank drain
  for the rendered-frame case; separately allow the explicit empty-display
  fallback test to expose the current second drain as a named failure.

The test should print the same explicit `PASS`/`FAIL` result style used by
nearby focused tests, and include a negative control that intentionally invokes
the old double-present sequence so the assertion is proven to kill that mutant.

## Status

`CONFIRMED_IN_CODE`: double-present path and positive-mode pacing no-op are
proven from current source. No production source was modified and no runtime
visual capture was required for this diagnosis. The exact compositor frame
timing (Wayland/XWayland scheduling) remains environment-dependent and is
secondary to the source-level defect.
