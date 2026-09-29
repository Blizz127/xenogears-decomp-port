# Xenogears PC port: purpose, standard and measured state

## Purpose

The port exists to **preserve** Xenogears: it runs the retail game 1:1, with
no changes or fibs. The game logic that runs is the decompiled retail code,
and the goal is that every retail function the port executes is the
byte-exact matched C that rebuilds the retail binaries.

Platform-layer design (interfaces, PsyCross backend, config, mods, cheats)
is in [port/ARCHITECTURE.md](port/ARCHITECTURE.md). Modding guide:
[port/MODDING.md](port/MODDING.md).

## The 1:1 standard

Only three kinds of non-retail code are allowed in what the port runs:

1. **The platform layer**, which faithfully reproduces the PlayStation
   hardware and BIOS/SDK behaviour the game relies on: PsyCross (GPU, SPU,
   CD, pads, timers), the clean-room `psyq_compat*` layer, and the `xg_plat_*`
   interfaces. It may change *how* the hardware is emulated, never *what*
   the game does.
2. **Opt-in mods**, off by default. With no mods installed every mod hook is
   a no-op ([port/MODDING.md](port/MODDING.md)).
3. **Cheats and test tooling**, off by default and only active when asked
   for (cheat console, `XENO_*` harness switches).

Everything else is retail game code. Where the port runs anything other
than the matched C for a retail function, that is either work still to do
(listed below) or a known divergence (listed below). Nothing is hidden.

## Classification (definitions, verbatim)

Each executed function is put in exactly one class:

- **matched C**: the function is in objdiff's byte-exact set
  (`build/progress.json` from `make report`, `fuzzy_match_percent == 100`,
  and the tree passes `make rom-check`), AND the body the port compiles is
  that same source, with no `#ifdef XENO_PC_PORT` / `#else` alternative arm
  inside the function.
- **matched C with port seam**: the same shared body, but with a small inner
  `#ifdef XENO_PC_PORT` for pointer width or host behaviour.
- **port-only C shadowing matched C**: the port runs a different body for a
  byte-exact function: a `XENO_PC_PORT` arm in the game TU, or a definition
  in `pc_port/` (including the world map's `wm_XXXXXXXX` for
  `func_XXXXXXXX`).
- **port-only C (no retail match yet)**: a port body for a retail function
  that has no byte-exact C yet.
- **shim/compat**: port infrastructure that is not a retail game function:
  `psyq_compat*`, `xg_plat`, the replaced Psy-Q SDK functions, harness and
  diagnostics helpers, generated stubs.
- **PsyCross**: the pinned PsyCross library.

The **matched-C ratio** counts matched C (with and without seam) over all
executed retail game functions, i.e. the first four classes. It is reported
by distinct functions and by executed calls. The strict figure (matched C
without seams) is given alongside.

Retail functions are the objdiff universe minus the Psy-Q SDK category.
Overlays share address ranges, so the byte-exact test for a function compiled
from a game TU uses that TU's overlay.

### How it is measured

- `XENO_COVERAGE=1 ./pc_port/build_port.sh` builds `pc_port/build_native_cov/`
  with `-finstrument-functions` on every game, port and PsyCross TU.
  `pc_port/tools/coverage_recorder.c` counts each function's calls and writes
  them to `$XENO_COVERAGE_OUT`. Run the coverage build twice: the battle
  bridge map uses the previous binary in the same directory as evidence.
- `tools/analysis/port_preservation.py coverage` classifies a coverage file
  using the link map, the objects' DWARF source names, a scan of each game
  TU's `XENO_PC_PORT` arms and `build/progress.json`.
- `tools/analysis/port_preservation.py shadows` lists every port body that
  shadows byte-exact matched C: [port/shadowing_matched_c.tsv](port/shadowing_matched_c.tsv).

## Measured state

Matching build (2026-09-29, integration merge `d169e0a5`): objdiff reports **2,651 / 4,464
retail functions byte-exact (59.39%)** and **381,600 / 1,383,684 code bytes
(27.58%)**. `make rom-check` passed all 10 pinned artifacts. The generated
progress report is [DASHBOARD.md](evidence/decomp-campaign/DASHBOARD.md).

The executed-route coverage below was measured on 2026-09-28 at integration
`9dddbd6b`; those route counts and ratios have not been rerun for `2ea6fce5`.

Routes (coverage build, retail disc):

| Route | What it reaches |
|---|---|
| R1 title | boot → title map 490 → interrupt attract → title menu → New Game → prologue map 4 → Lahan map 2 (`run_title_newgame_smoke.sh`, 68 captures; passed 2026-09-29) |
| R2 field | `XENO_FIELD_TEST=1` developer field entry, 60 s |
| R3 field → battle | `XENO_FIELD_TEST=1 XENO_FIELD_MAP=1`, battle 1 via `XENO_BATTLE_WARP_FILE`; `run_static_data_cwd_test.sh` enters retail `battle.bin` from an unrelated cwd and runs it for the 35-second regression window (D5) |

Executed functions by class:

| Class | R1 title | R2 field | R3 battle | all routes |
|---|---:|---:|---:|---:|
| matched C | 196 | 230 | 301 | 349 |
| matched C with port seam | 21 | 23 | 25 | 26 |
| port-only C shadowing matched C | 26 | 29 | 30 | 32 |
| port-only C (no retail match yet) | 110 | 149 | 159 | 198 |
| shim/compat | 226 | 214 | 227 | 281 |
| PsyCross | 310 | 294 | 287 | 322 |

**Matched-C ratio** (executed retail game functions):

| Route | by functions | by calls | strict (no seam), by functions |
|---|---:|---:|---:|
| R1 title | 217/353 = 61.5% | 76.1% | 196/353 = 55.5% |
| R2 field | 253/431 = 58.7% | 66.2% | 230/431 = 53.4% |
| R3 battle | 326/515 = 63.3% | 68.4% | 301/515 = 58.4% |
| all routes | 375/605 = 62.0% | 71.6% | 349/605 = 57.7% |

Battle code that runs in the MIPS interpreter is retail machine code from
the disc. It is not counted in these tables, which count native functions
only; only the interpreter itself appears, as shim.

### Port bodies shadowing matched C

260 port bodies shadow 256 byte-exact functions
([port/shadowing_matched_c.tsv](port/shadowing_matched_c.tsv)):

| Where | Bodies |
|---|---:|
| `XENO_PC_PORT` arms in game TUs: battle 69, slus 28, field 16, menu 10, movie 1, shop menu 1 | 125 |
| `pc_port` bodies, including world-map callbacks and adapters | 111 |
| generated logging stubs standing in for matched C that is not linked | 24 |

Their equivalence to retail is not proven. They are switched to the matched
C in route-tested batches (see below). Until then each one is a potential
divergence.

The three board operations `func_8007B2C0`, `func_8007B310` and
`func_8007B360` now share their retail-matching C bodies, with an inline
retail/host row-address adapter. Each compiles to the exact 20 retail words.
`func_8007A874` also moved from a port-only body into the matched battle TU.
Its retail output is exact (16/16 words); the existing remu differential passed
238 seeds at O0, O2 and UBSan, and rejected the row-offset mutant.
`func_8007AEF0` now shares its retail body, using compiler-pinned MIPS
registers and a host declaration seam. It compiles to the exact 27 words; the
remu differential passed 238 seeds at O0, O2 and UBSan and rejected the add
mutant.
`func_8007B578` now also uses the shared matched C body. Its 36 MIPS words are
exact; the existing remu differential passed 256 seeds and rejected the
wrong-destination mutant.
`func_8007B98C` and `func_8007B9C8` now share their matched C bodies through
inline row-address adapters. Both are exact at 15/15 words; the combined
remu differential passed 747 checks at O0, O2 and UBSan and rejected its
copy-direction mutant.
`func_8007BA44` now shares its matched body with a pointer-width adapter; its
17 retail words are exact, and the remu differential passed 238 seeds at O0,
O2 and UBSan while rejecting the destination mutant.
`func_8007BA04` now shares the matched body as well. It is exact at 16/16
words; its remu differential passed 261 seeds at O0, O2 and UBSan and rejected
the wrong-slot mutant.
`func_8007BAE8` now also shares its matched body. It is exact at 17/17 words;
the existing remu test passed 480 seeds at O0, O2 and UBSan and rejected the
slot-offset mutant.

## Known divergences (public list)

Behaviour that differs from retail and is known. Detailed history of every
found-and-fixed case: [port/RETAIL_DIVERGENCES.md](port/RETAIL_DIVERGENCES.md).

| # | Area | Divergence | Status |
|---|---|---|---|
| D1 | field `func_800ACB90` | previously used y=0 for its TIM upload and wrote its 0x200-byte clear buffer past the allocation | fixed against retail assembly arguments and clear-loop addresses; `run_field_acb90_retail_test.sh` checks both behaviors and rejects old-value mutants; retail function remains `INCLUDE_ASM` |
| D2 | menu `func_801CD710` | retail returns 0 when the case handler returns nonzero; the handlers (`func_801CD2AC` / `CC6D8` / `CB304` / `CBD90`) have no port body, so the result is not used | partly fixed (433db88e) |
| D3 | slus `MenuExecute` debug branch | the developer KernelMenu route reserves/loads nothing where retail reserves the heap top for two overlays (the port runs them natively) | by design for that developer-only route; retail bytes unchanged |
| D4 | CD / movies | XA ADPCM audio sectors (read mode 0x200, 2340-byte sectors) are refused (fail-closed), so XA audio is not played | open: needs the XA sector path in the platform layer |
| D5 | static data / battle | when launched outside the project root with `XENO_DATA_DIR`, the loader left main-exe `.rodata` / `.sdata` zero-filled; battle then stopped in DrawOTag | fixed: shared disc-layer search order; `run_static_data_cwd_test.sh` and `run_static_data_package_test.sh` |
| D6 | title test harness | the input schedule used physical button bits while documented as game button bits, so Circle did not interrupt the attract movie in the smoke | fixed: map-layout button translation; real-disc Xvfb smoke with no BIOS reaches New Game and Lahan, with no fatal markers |
| D7 | world map | runs hand-written `wm_*` C rather than the retail overlay code (world-map switch batches 1-6 moved 42 scheduler callbacks to the byte-exact C; the rest still run `wm_` bodies) | open: switch batches below |
| D8 | missing bodies | 72 retail functions still have only a generated logging stub, which returns without retail behaviour; 18 of them have matched C that the port does not link | open |

Options that are reported as unsupported and ignored, never faked:

| # | Option | Status |
|---|---|---|
| U1 | `video.widescreen` | unsupported, 4:3 used |
| U2 | `video.fps = 60` | unsupported (needs interpolation), 30 used |
| U3 | user output volume | no backend control; PsyCross master volume is the game's own `SpuSetCommonAttr` value |

Untested: routes beyond R1–R3 (story progress past the prologue, world map
entry from play, save/load via the memory card UI). Asset replacement for
textures and sound samples is covered by tests only through the identity
mod on boot; streamed-file replacement has a live test.

**Known-divergence list: 8 tracked items (D1–D8; D1, D5 and D6 fixed, D3 by design,
the others open or partly fixed), plus 3 unsupported options (U1–U3). A
further 256 matched functions run port bodies of unproven equivalence.**

## Switching shadowing bodies to the matched C

Each batch is one commit:

1. Make the port compile the matched body: drop the port arm, or turn the
   `pc_port` body into a call, keeping only host/pointer seams as small inner
   `#ifdef XENO_PC_PORT` blocks in the shared body (so the retail bytes stay
   exact).
2. Verify: port build, guards, `make rom-check` 10/10, the TU's port runners,
   and a route that executes the functions (coverage file shows them).
3. Re-measure: the function moves to "matched C" or "matched C with port
   seam" in this document.

The world map batches follow `wm_XXXXXXXX` ↔ `func_XXXXXXXX` (63 of the 97
matched functions in the slus mapping have a `wm_` counterpart) and are coordinated with the slus
agent, which adds any inner seams in `src/world_map/main.c`. They are
verified by the world-map port tests (`run_world_map_differential_test` and
the others) plus a route that enters the world map.
