# Port status

The port is `pc_port/`. It is a native program. It compiles the decompiled C with `-DXENO_PC_PORT -DSKIP_ASM` and links [PsyCross](https://github.com/OpenDriver2/PsyCross) for the PlayStation hardware calls: software GTE, LibGPU to OpenGL, LibSPU to OpenAL, LibCD to a disc image, controllers to SDL2. PsyCross, the disc, and the BIOS are obtained locally. They are not in this repository.

`pc_port/build_port.sh` is the driver that produces `pc_port/build_native/xeno-port`. The standalone `cmake -S pc_port` target is only the PsyCross scaffold. It does not compile the game translation units.

## What the link contains

| | Count | Meaning |
|---|--:|---|
| C files under `src/` | 275 | Decompilation |
| Psy-Q sources | 46 | Excluded by `*/psyq/*`. PsyCross and `pc_port/src/psyq_compat.c` supply the hardware side |
| Intentionally excluded | 2 | `src/slus_006.64/system/archive.c` (replaced by `archive_port.c`) and `src/battling/main.c` |
| Reference-only | 169 | Almost all of `src/battle`. Present for the matching build. Not compiled into the port link |
| Known compile-failure allowlist | 0 | `KNOWN_BROKEN_GAME_TUS` is empty. A new game-TU compile error stops the build |
| Game TUs compiled into the port | 58 | Field, menus, shop, movie, member change, and the rest of the main executable |
| Port-only C files | 268 | `pc_port/src`, 79,689 lines |
| Port-owned overrides | 22 | `pc_port/port_owned_overrides.txt` |
| Test files under `pc_port/tests` | 1234 | Harnesses and oracles. Not the game |

`INCLUDE_ASM` becomes empty under `SKIP_ASM`. A symbol that is still undefined is filled by a logging stub generated at build time. Those stubs are not in git. A stub returns without the retail behavior and logs the name.

## What is actually running

Field code is on the native side. The field overlay's 878 functions are matched C. Walkmesh, collision, the script VM, and the routes exercised in this tree (Lahan and Blackmoon Forest) go through that C. Save and load of a field checkpoint is `F7` / `F8` (`quicksaves/quick.xgqs`). `F9` records the framebuffer.

Battle is not those 169 retail TUs. `pc_port/src/battle_mips_adapter.c`, `battle_mips_runtime.c`, and `battle_overlay_host_leaves.inc` run an allowlist of battle leaves as native C. Anything else in `disc/battle.bin` is interpreted. Of the battle `INCLUDE_ASM` lines, 506 have no C body in the port compile.

The world-map mode can be reached from a field exit. The retail world-map overlay is not what that mode runs.

Menu coexistence is in better shape than battle: 80 menu `INCLUDE_ASM`s have a port C body next to them, and 59 are omitted. The main executable has 26 with a body and 35 omitted.

## Overrides

A name in `pc_port/port_owned_overrides.txt` is a port function that owns the symbol until the matching C is safe to link on the host. The file lists 22. Each row says why, and what has to be true before the port copy is removed. Two definitions of the same name are a link error. That is intentional.

The mechanisms (skip-asm stubs, `XENO_PC_PORT` branches, whole-TU replacement, overrides) are written up in [Decomp and port coexistence](Port-Coexistence-Architecture).

## What a port change is

A new port commit should add or fix C under `pc_port/`, or fill in a port body for a function that is still omitted. Byte-exact matching of an `INCLUDE_ASM` that already has a correct port body does not change the PC binary. A battle function with no body does: until it exists as C, that call is a stub or an interpreter fallback.
