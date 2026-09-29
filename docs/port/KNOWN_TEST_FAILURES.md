# Known port test failures

Status of the `pc_port/tests` runners that are not green, with their cause.
Anything cheap to fix has been fixed; what remains here needs an owner or a
decision. Update this file when a failure is fixed or a new one is accepted.

## Test environment

The runners assume the dev-host toolchain. In the docker helper use the
`xenogears-dev:clang` image (`XIMG=xenogears-dev:clang ~/.cache/xeno/dbuild.sh …`),
which adds to the port image:

- **gcc ≥ 14** as `gcc`/`cc`: many runners pass `-fpermissive` to C compiles;
  gcc 13 reports it as a warning ("valid for C++ but not for C"), which
  `-Werror` makes fatal. gcc 14+ accepts it (the dev hosts run gcc 15).
- **clang** (UBSan regimes), **ripgrep** (`rg` assertions), **gdb**
  (`run_kernel_menu_route_smoke.sh`).
- GNU coreutils: runners must not `cat` a file into itself (uutils `cat`
  allows it, GNU `cat` refuses).

With that image, the 52-runner battery that links the data/compat TUs plus the
field/menu/controller runners used for integration verification are green
(65/65 on 2026-09-28).

## Currently failing

None.

## Fixed on 2026-09-28 (for reference)

- `run_kernel_menu_route_smoke.sh`: the MenuExecute debug-branch heap
  corruption, fixed in slus a5e5fcee.
- Link gaps in tests that link a TU without its port collaborators: the port
  hooks are now weak references (`PcPort_GodModeBeforeGuest`,
  `g_PcPortPresentedFrames`, `PcPort_RandomBattlesEnabled`,
  `PcPort_WalkmeshDump`/`FieldPosDiag`/`FieldCaptureOnVsync`, the vblank
  service calls in `psyq_compat.c`, the retail-data pointer mapping).
- Extractors cut by the one-body `#ifdef` layout or the temp1 split
  (menu equip_desc / gear_caller / scrollbar_layout, battle_child_tile,
  anim_render_index15 asm paths, battle_guest_call asm path).
- Stale assertions: menu_file_host_boundary (now `PcPortCard_*`),
  battle_overlay_bootstrap (`generate_asm_macros_files` must not be `True`),
  sprite_opcode_a9 (weaken temp1's new `func_80023B84`), battle_graphics_abi
  (bridge cache: check truncation on a fresh runtime), battle_gte SetMulMatrix
  (compare VZ0–VZ2 as 16-bit registers, as the hardware exposes them).
- Test build hygiene: `-fpermissive` only on game-TU compiles (98430, 97a50),
  unchecked `write()` under `-Werror`, `-U_FORTIFY_SOURCE` for a fixture that
  defines `bzero`, battle TUs compiled with `XENO_BATTLE_OVERLAY_HOST_BODIES`,
  `scratchpad/` created by the runners that use it (and git-ignored),
  load_image_cache's log self-`cat`, clang looked up on `PATH`.
