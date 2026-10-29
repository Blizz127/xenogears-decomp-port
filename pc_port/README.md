# Xenogears PC Port (experimental, Shipwright-style)

A native PC source port built **on top of the matching decompilation**, in the
spirit of [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) for
Zelda OoT and the [Silent Hill PC port](https://github.com/SlickAmogus/silent-hill-decomp)
for PSX. This directory is **separate from the matching (gears/ninja/MIPS)
build** and does not affect it.

## Architecture

```
  Xenogears game logic (../src/*)         <- decompiled C
            │  calls the PsyQ SDK (DrawOTag, RotTransPers, Spu*, CdRead, ...)
            ▼
  PsyCross  (PSX hardware abstraction layer)
    GTE in software · LibGPU -> OpenGL · LibSPU -> OpenAL ·
    LibCD -> BIN/CUE · controllers -> SDL2
```

PsyCross is the PSX equivalent of libultraship (the N64 HAL behind SoH). The
port compiles the game against PsyCross's PsyQ-compatible headers
(`extern/PsyCross/include/psx`) and links its implementation, **instead of** the
on-hardware reimplementations in `../src/slus_006.64/psyq/*`.

## Status

The default build remains **hybrid**: boot and field execute host C, while
unconverted battle paths use the CPU interpreter. The ongoing
[native-conversion milestone](../docs/port/NATIVE_CONVERSION.md) tracks the
opening route, missing functions and acceptance evidence. Function counts do
not establish native gameplay coverage.

The selected local native-only checkpoint (`573e8f72ef41ea43...`)
crosses the complete animation-stream leaf, then refuses `800BB350` at logged
site `801E65D8` in the native startup-resource loader. The CPU decoder is absent
and observed interpreted instructions are zero. GCC O0/O2 and Clang UBSan pass
the bounded leaf tests; a same-executable disabled control restores `800AE1BC`.
Four pre-battle captures and all 9,052 raw game-state bytes exactly match the
prior candidate. The prior name-tail repair and tested coverage shader are
retained. This executable remains selected and contains no scene-coverage
producer.

The coverage producer and lifecycle repairs are now linked into separate graphics
candidate5 (`2bfce9cd96121dda...`), which remains **unselected**. The selected
`573e8f72ef41ea43...` executable is unchanged. One map4 aerial-village sample
changes 28,671 RGB pixels within 73,615 certified world pixels; all 3,185
protected pixels stay identical. The other ten samples remain protected and
unchanged within the pass. All eleven paired passes preserve CPU VRAM. No
positive map2 sample or visit to maps14/1 is established. Master OFF remains
the default; coverage supplies no depth.

The nine sampled map2 ON/OFF comparisons contain no novel VRAM offset/value
pairs beyond the startup delta. Strict full-VRAM parity still fails, including
unresolved startup and step3000 differences. At step100 both candidate modes
show different partial opening text while the selected reference is visually black;
no cause is established. OFF/reference captures and swapped RGBA match the
other ten checkpoints, and all eleven raw GameState records match, but OFF
identity and the present-hash coverage policy still fail. Candidate5 binary-data
and strict SDK checks, bounded pending-capture tests and clean vendor replay
pass; overall graphics status remains **INVESTIGATE**.

The BB350 parent and five dependencies pass GCC O0/O2 and Clang UBSan fixtures
across an explicit native heap/work-list service boundary. Their source is now
integrated with the parent opt-in. Unselected native candidate
`0127f25b91a81088...` reaches `801E6710` from `801E703C` with zero interpreted
instructions and the decoder absent. Its four opening captures and 9,052-byte
state capture match both controls. Paired passive snapshots support five new
sprite allocations and matching native task-list entries. Callback bodies
remain unconverted. Graphics candidate5 retains the 573e runtime.
The prior unselected startup candidate `a093e6ed94725dba...` reaches `801E693C`
from `801E7078` when `XENO_BATTLE_NATIVE_STARTUP_ACTORS=1` and the existing
parent opt-in are both enabled. The CPU decoder is absent and interpreted
instructions remain zero. Both disabled controls reproduce prior stops;
all four captures and 9,052 state bytes match. The slot loop returns on this
route without exercising slot construction. The binding and constructor
branches retain their narrower fixture coverage; this is not a complete battle.
The newer unselected startup-queue candidate `9adbc06e0de3bc53...` adds the
complete `801E693C` caller and explicit native archive/heap service boundaries.
Its normal opening, with queue, startup-actor and parent controls enabled, first
refuses `80076544` from `800BE90C`, with the CPU decoder absent and zero
interpreted instructions. The queue-disabled control restores `801E693C` from
`801E7078`, and the parent-disabled control restores `800BB350` from `801E65D8`.
All four strict comparisons preserve every capture and all 9,052 state bytes.
Matched routes and pinned source ordering support return from `801E693C` and its
enclosing `801E6FEC` callback; this is an inference, not a per-entry execution
trace. No new allocation or `801E6F00` execution is established. Selected
`573e8f72ef41ea43...` is unchanged. This candidate retains the selected build's
older renderer; graphics candidate5 retains the older native runtime.

Graphics descriptor checkpoint7 completes both diagnostic collections, but
all four new same-mode present comparisons fail the existing pairing policy
(about 39% scene coverage). All eleven state samples match; full VRAM differs
at every checkpoint. Exact context/structure matching yields zero metadata
pairs, so no descriptor-field or causal-writer attribution is established.
Graphics remains **INVESTIGATE**; the matching policy is unchanged.

See the [queue checkpoint](../docs/port/NATIVE_CONVERSION.md#startup-queue-runtime-checkpoint--2026-10-20)
and [descriptor checkpoint](../docs/port/NATIVE_CONVERSION.md#graphics-descriptor-diagnostic-checkpoint--2026-10-20).
No candidate package has been created or selected.
See the [startup checkpoint](../docs/port/NATIVE_CONVERSION.md#startup-actor-runtime-checkpoint--2026-10-20).
See the [candidate5 checkpoint](../docs/port/NATIVE_CONVERSION.md#pending-capture-graphics-candidate5--2026-10-20).
This remains an incomplete native port. Owner-device checks remain NOT RUN
and shipment is held.

- [x] **Phase 0 (scaffold):** PsyCross vendored (`extern/PsyCross`, gitignored)
      and building from source; `xeno-port` links against it and brings the
      runtime up/down.
- [x] **Phase 1 pattern (proven on a slice):** a real game translation unit
      (`src/field/game_logic/gold.c`) compiles natively in port mode, its
      undefined symbols are auto-stubbed, and the linked binary **runs real
      decompiled game logic on x86**, with the stub oracle logging the next
      function the code path needs. See "Phase 1 loop" below.
- [ ] **Phase 1 (scale-up):** compile *all* game TUs, reconcile the full PsyQ
      header surface, link the whole executable, hand `main` off to the game.
- [ ] **Phase 2:** asset/disc extraction pipeline feeding PsyCross's LibCD.
- [ ] **Phase 3:** climb the boot path, replacing stubs with decompiled C until
      the intro/field renders.

### Battle RESULTS module: adoption status (CPU fallback disclosed)

The battle results module (retail overlay, disc archive 0x10/file 4, linked at
guest 0x801DE000, 0x6500 bytes) still runs **interpreted** by
`src/battle_mips_runtime.c`'s MIPS interpreter, except for the routines listed
in `src/battle_results_host_leaves.inc`, which run as native C from
`src/battle_results_host.c`:

| Routine | Status |
|---|---|
| `func_801E24B0` (result-id -> g_GameState name-table map) | native, identity-gated; differential PASS on the runner (gates 3-7) |
| `func_801DF910`, `func_801DFA38`, `func_801E1590`, `func_801E2888`, `func_801E403C`, `func_801E41B4` (leaves batch 2: percent bar, row max, drop dedupe, displayed-value clamp, record-state promotion, level badges) | native, identity-gated; differential PASS on the runner (gate 7, 70b48dc5) |
| `func_801E1370`, `func_801E2EB0`, `func_801E3BE0`, `func_801E3D54`, `func_801E3E14`, `func_801E3EA4`, `func_801E3F28`, `func_801E3FB0` (leaves batch 3: inventory table add, EXP/SP split, the six 0x801E3A18 skill/flag helpers) | native, identity-gated; differential PASS at O0/O2/UBSan on the runner ([batch-3 evidence](../docs/evidence/battle-results-batch3-20261002.md), source `6e189169`) |
| `func_801E3A18` (skill/flag promotion over the three result slots; the first adopted non-leaf: writes its guest frame and calls the six batch-3 helpers directly) | native, identity-gated; contract oracle + composition PASS (gate d1d329f4); leaf differential pending the next gate ([contract](../docs/evidence/results-3a18-contract.md)) |
| every other results routine (entry 0x801E252C, flow controller, their callers such as 0x801E1444, EXP/AP/level/item steps, number/sprite drawing, ...) | **interpreted** (CPU fallback) |

A native body runs only while the module's immutable bytes
0x801DE000..0x801E44C0 in guest RAM match the pinned retail SHA-256
(`src/battle_results_gate.inc`); a different overlay at that address, any
modified byte, a pending delay-slot load or the `force_interpret` test hook
makes the runtime interpret the retail bytes instead. Bodies reach guest
memory only through `PcPort_BattleMipsResolve` (the interpreter's own address
mapping, so main-exe globals such as `g_GameState` hit their native copies).
`XENO_BATTLE_RESULTS_DIAG=1` logs identity verdicts and the first native run
of each routine. Not a native-conversion claim for the battle: the battle
overlay still has many interpreted paths beyond the adopted leaves and
lifecycle routines described below.

The battle entry `func_80070F40`, startup dispatcher `func_800B8098`, display
initializer `func_800B8284` and screen transition `func_800B7870` have
identity-checked host C bodies in
`src/battle_{entry,startup,display,transition}_host.inc`. Their callees are converted
separately. Differential tests cover modeled call boundaries; the display
tests also exercise the actual native SDK initializers through the production
bridge. These checks do not prove a complete battle.
`XENO_BATTLE_NATIVE_ENTRY=0`, `XENO_BATTLE_NATIVE_STARTUP=0`,
`XENO_BATTLE_NATIVE_DISPLAY=0` and `XENO_BATTLE_NATIVE_TRANSITION=0` select their
old paths for reference tests. The transition tests cover loader delays,
double buffers, callback changes, scratchpad stack switching and the caller's
Vsync cadence with modeled callees.

`src/battle_shard_setup_host.inc` adds shard allocation `func_800B73EC` and
initialization `func_800B7424`, preserving the returned task pointer, all 560
shards, RNG order and guest primitive layout. Tests compare both modeled
callees and the actual native SDK/RNG through the production bridge.
`XENO_BATTLE_NATIVE_SHARD_SETUP=0` selects their reference paths. The normal
boot route completes these native bodies with zero interpreted instructions
and enters the loaded module's setup. The transition animation and opening
battle remain incomplete in the native-only target.

`src/battle_module_setup_host.inc` implements the archive 0x0C/file 4 loader
dispatcher `0x801E5840` and first-stage setup `0x801E5384`. Both code hashes
must match before either body is adopted at these overlapping overlay
addresses. It preserves party filtering, character/gear resources, the
seven-argument upload and scenario-file queue descriptors. Other module
stages and callees remain separate conversion work.
`XENO_BATTLE_NATIVE_MODULE_SETUP=0` selects the reference path. The normal
boot run now completes its first-stage resource work with zero interpreted
instructions. Later stages remain unconverted. All 112 modeled-callee
differential cases pass at O0/O2/UBSan, with ten negative controls.

`src/battle_texture_host.inc` adds `0x80078310`, preserving the packed 20-byte
guest TIM, 0xC8-byte frame, eight helper arguments and CLUT/pixel upload order.
The bridge calls OpenTIM, DrawSync and the atlas getter with their actual C
signatures and resolves physical RAM pointers explicitly; LoadImage also uses
its exact signature. `XENO_BATTLE_NATIVE_TEXTURE=0` selects the reference path.
Tests pass 64 modeled and 32 native SDK integration cases at O0/O2/UBSan, plus
twelve negative controls. The SDK tests use a recording graphics backend.
Normal boot completes the uploader with zero interpreted instructions.

`src/battle_shard_callbacks_host.inc` adds shard update `0x800B6F0C`, render
`0x800B7160` and cleanup `0x800B7330`, preserving real scratchpad frames,
signed velocity damping, packet buffers, projection arguments and state restore.
The graphics boundary keeps the game's Q12 matrix rounding and 24-bit game
addresses in ordering-table links. `XENO_BATTLE_NATIVE_SHARD_CALLBACKS=0`
selects the reference path. The callback checkpoint's opening probe completes update/render and then
refuses loader-stage call `0x801E4048` at `0x801E589C`, with zero interpreted
instructions. See the milestone for exact candidate evidence and the remaining
gameplay/visual/audio/timing/save limits.

`src/battle_module_stage1_host.inc` implements the next loader stage: state
reset `0x801E4048`, slot/group/position setup `0x801E4160`, enemy resources
`0x801E4870`, and actor modes `0x801E4AC0`. Each body requires its own loaded
code identity and the dispatcher identity. `XENO_BATTLE_NATIVE_MODULE_STAGE1=0`
selects the reference path. The loader preserves live data reads, packed
records, real caller frames, optional resource flags and the actor-7 gear
exception. Its memory and slot-mask calls use explicit host signatures.
The party-stat and special-mode callees are implemented separately below.
The normal boot probe completes all four stage-one routines with zero
interpreted instructions. The battle and transition are still incomplete.

`src/battle_party_stats_host.inc` adds party-stat initialization `0x80097D5C`
and the optional special-mode adjustment `0x8009B098`. Both check their entire
loaded code regions and preserve packed 32-bit pointers, byte/halfword wrapping,
stat caps, character/equipment rules and arithmetic HI/LO results.
`XENO_BATTLE_NATIVE_PARTY_STATS=0` selects the original reference path.
The test suite compares 192 stat cases, 48 special-mode cases and 64 composed
actor-mode/stage-1 cases against the original bodies with actual libc and the
compiled mask helper. No stat callee is modeled in that suite.

`src/battle_module_stage2_host.inc` implements inventory setup `0x801E4CD0`,
turn order `0x801E4E7C` and command-menu setup `0x801E5014`, with each body and
the resident dispatcher identity checked. Packed item lists, original list
bounds, random retries, signed timer arithmetic, real frames and live menu
pointers are retained. `XENO_BATTLE_NATIVE_MODULE_STAGE2=0` selects the
reference path. Its 192-case suite uses modeled game callees and passes at
O0/O2/UBSan, including 13 deliberate-fault checks. With the turn-speed and
target-list bodies below, normal boot completes inventory, turn ordering and
command setup, then reaches loader stage three with zero interpreted
instructions. Later loader stages and battle completion remain open.

`src/battle_turn_speed_host.inc` implements reset `0x80078508`, speed
`0x80098AF8` and flag repair `0x80094D24`, each behind a loaded-code identity
check. It replaces the legacy reset boundary with original frames, live bus
accesses and structured refusal propagation. Signed equipment penalties,
RNG spread, timer stores and branch-dependent returns are retained.
`XENO_BATTLE_NATIVE_TURN_SPEED=0` selects the reference path. The bounded RNG
bridge uses its declared byte arguments/return; the PC body now defines the
original zero-divisor remainder instead of performing undefined C division.

The suite passes 928 game-body comparisons at O0/O2/UBSan, including 96
integrations through actual native RNG services. Each configuration also
compares 512 original RNG seeds and all 65,536 byte-range argument pairs.
The RNG service contract covers returns and seed, not original SDK frames or
volatile CPU state. All 14 deliberate faults are detected, and the existing
192-case stage-two regression passes. That checkpoint (`2d0525fa15d09698...`)
reached command setup's target-list refusal with the CPU decoder absent.
Its private receipts remain under `scratchpad/native-turn-speed-20261018/`.

`src/battle_target_list_host.inc` implements target-list construction
`0x800841E0`, eligibility `0x80083FF4` and priority comparison `0x80085310`.
Each loaded body has its own identity check. The list retains its original
80-byte frame, eligibility call sites, live aliases, stable group partition
and unsigned rank swaps. Eligibility uses the production memory bus and
preserves normal/alternate status branches and the original V1 return.
`XENO_BATTLE_NATIVE_TARGET_LIST=0` selects the reference path.

The suite passes 1,056 original-body comparisons per O0/O2/UBSan configuration,
including 32 complete command-setup callers with no modeled game callees.
All nine deliberate faults are detected. The O2 turn-speed regression also
passes 928 game-body cases, 512 RNG seeds and all 65,536 byte-range pairs.
The candidate `f143364be82739ca...` completes stage-two setup on the normal
boot/New Game/prologue/Lahan route and refuses stage three's `0x801E6290`
at `0x801E5904`, with zero interpreted instructions. Outer counters and state
hashes are retained. Full battle, visual/audio/timing/save and owner-device
acceptance remain open. See `docs/port/NATIVE_CONVERSION.md` and private
`scratchpad/native-target-list-20261018/` receipts for exact scope.

`src/battle_module_stage3_host.inc` adds the six stage-three loader bodies:
UI primitives `0x801E5924`, party glyph setup `0x801E5EE8`, panels
`0x801E5D2C`, text buffers `0x801E5E78` and wrappers `0x801E6290` /
`0x801E62B8`. Each requires its own complete loaded-code identity and the
resident dispatcher identity. Live owners, signed coordinates, original
frames, packed counters and nested refusal boundaries remain on the memory
bus. `XENO_BATTLE_NATIVE_MODULE_STAGE3=0` selects the reference path.

At O0/O2/UBSan, the suite passes 224 caller comparisons with controlled game
services and 112 integrations with the actual atlas getter/SDK packet setters.
Glyph, allocation and text-rendering services remain models in this suite.
An independent suite passes 512 original SDK packet/return comparisons per
configuration, scoped to graph type 0; service CPU frames and volatile state
are excluded. All 12 deliberate faults across the two suites are detected.
The O2 target-list regression passes all 1,056 cases without modeled game
callees. Typed SDK boundaries preserve void returns and the real fifth
SetDrawMode argument; GetClut packing uses defined unsigned arithmetic.

`src/battle_glyphs_host.inc` adds the full/half glyph wrappers at
`0x80076A10`/`0x80076A6C` and the atlas renderer at `0x8002675C`. It preserves
original frames, signed coordinates, Q12 rounding, HI/LO, flips, live counts
and overlapping packet writes. Wrapper identities use loaded battle bytes;
renderer identity uses the existing verified asset cache because main CPU
text is deliberately absent from game RAM. `XENO_BATTLE_NATIVE_GLYPHS=0`
selects the reference path. `SetPolyFT4` has a concrete typed SDK boundary.

The new suite passes 288 original-body comparisons and 64 original FT4 packet
checks in each of O0/O2/UBSan, with actual atlas/style/SDK services and zero
native interpreted instructions. All ten deliberate faults are detected.
The prior stage-three suite passes its O2 regression: 224 modeled-callee
cases, 112 actual atlas/SDK cases with typed game-service models, and 64 CLUT
returns. Those game models now bind directly to the test's typed boundary.
SDK CPU frames/volatile state and full display/audio/timing/save behavior
remain outside these bounded tests.

The historical local candidate `1f3cbd25090c56b6...` follows normal boot/New Game
through Lahan and battle setup, passes the previous motion-slot lookup stop,
then refuses the next unconverted target `0x800a1b50`. The CPU decoder is absent
and observed interpreted instructions remain zero. Full native battle and
whole-game acceptance remain incomplete.

The complete 352-byte motion-slot parent and 144-byte status child now execute
compiled host C. They preserve byte-truncated codes, the initial output clear,
actual child state, live aliases, raw 0x80 flags, both pointer-table formulas,
unsigned health comparisons and delayed-load failure prefixes. Complete loaded
identities precede legacy caches. The actual CallJal interface is checked with
its separate CPU and real children.

GCC O0/O2 and Clang UBSan each pass 3,156 direct body comparisons, 1,599 ordered
access failures, 2 child refusals/pending returns, 510 identity/context gates,
24 cache checks, 26 independent branch/register/alias/domain witnesses,
289 actual CallJal cases/refusals and 116 ordered CallJal failures.
Nineteen compiled defects are detected. Twenty forwarding controls pass
1,771 combinations. Native test legs execute zero interpreted instructions;
original instruction execution is confined to the test oracle.

Four pre-battle captures match the previous checkpoint. Raw gamestate has
12 differing bytes; all after name terminators:
`True`. No normalization is used.
Five older fixtures still need supplemental shared-memory coverage; the current
suite independently checks real lower shared backing. Exception/pipeline
resumption and broad graphics/audio/timing/save acceptance remain open.
Full source-data fails on two unchanged runs/80 bytes; changed sources pass.
Six unchanged address notes total 876 bytes. Linked data fails on two runs/84
bytes. SDK exclusion passes; its table scan is SKIPPED. Owner Alienware/Bazzite
controller, borderless window and automatic keyboard checks are NOT RUN,
with device identity unknown. Shipment remains held.
Evidence: `scratchpad/native-motion-slot-20261020/` and
[`docs/port/NATIVE_CONVERSION.md`](../docs/port/NATIVE_CONVERSION.md).

The newer local development candidate `04fa1c100c5f6a60...` implements the
complete 420-byte motion-triple target and reaches `800AF438` on normal
boot/New Game. A same-executable disabled control restores the prior refusal,
with all four captures and raw game state identical. The decoder is absent.
GCC O0/O2 and Clang UBSan pass 692 body cases, 2,856 ordered faults,
identity/cache checks and the actual CallJal interface, with zero interpreted
instructions on native legs. The historical motion-slot candidate above is
preserved as a rollback output.

Eight numeric HD2D intensity controls support scrolling and horizontal input.
Menu/bridge model tests pass O0/O2/UBSan, the actual SDL virtual-pad suite
passes, and rendered menu captures cover 320×240, 640×480 and 1280×720.
Composition still lacks production scene inputs, so these controls do not
establish visible new effects. The SDK matrix-stack repair passes its tests.
Fresh source and binary data scans pass; six unchanged address facts remain.
The two small historical capture differences and raw state differences are
recorded without normalization. Full native battle and audio/timing/save
acceptance remain incomplete. Owner-device checks remain NOT RUN and shipment
is held. Evidence: `scratchpad/native-motion-triple-runtime-20261020/` and the
[runtime checkpoint](../docs/port/NATIVE_CONVERSION.md#motion-triples-and-graphics-controls-runtime-checkpoint--2026-10-20).

### Battle CPU-fallback meter (`XENO_CPU_FALLBACK_STATS`)

How much of a battle still runs on the MIPS interpreter is measurable at
runtime. With `XENO_CPU_FALLBACK_STATS=1` (unset or `0`: off, nothing counted
or printed, dispatch unchanged) the battle runtime counts per battle --
`func_80070F40` entry to exit -- and prints to **stderr** at battle exit:

```
[xeno-port][cpu-fallback] battle=1 end=returned interpreted_instructions=N interpreted_calls=N interpreted_targets=N untracked_calls=N native_main_exe_calls=N native_overlay_calls=N native_results_calls=N native_file1_calls=N native_call_share=P%
[xeno-port][cpu-fallback] top 16 of N interpreted call targets:
[xeno-port][cpu-fallback]    1. 0x800xxxxx calls=N (P%) battle.bin
```

- `interpreted_instructions`: guest instructions the interpreter executed
  (the disclosed CPU fallback), from every guest CPU of the battle (the main
  run, guest calls from native code, work-list callbacks).
- `interpreted_calls` / the table: guest calls that ran interpreted -- a
  JAL/JALR linking `ra` that the bridge did not take natively, plus each guest
  entry made from native code -- by target address.
- `native_main_exe_calls`: guest calls bridged into native main-executable
  code; `native_overlay_calls`: battle.bin/overlay addresses run as native host
  C (`src/battle_overlay_host_leaves.inc` and the typed overlay bridges);
  `native_results_calls`: adopted RESULTS bodies (above);
  `native_file1_calls`: the adopted file-1 controller 801E6CE8.

Enabled, it only reads guest memory (the call word behind `ra`) and updates
counters: no guest state, register, dispatch decision or timing input
changes. Call detection is a heuristic (a JALR through a register other than
`ra` is not counted as a call; its instructions are). Implementation:
`src/battle_cpu_fallback_meter.inc`; self-check: the `--meter` run in
`tests/run_battle_results_host_differential_test.sh`, which checks the counts
against `cpu.steps` and the known call sequence of 0x801E3A18 and that the
normal run prints nothing.

The batch-3 runner self-check passes in all three compiler modes. It exercises
one interpreted caller and nine native results-helper calls, not a complete
battle. `native_call_share` is a share of calls, not execution time or instruction
coverage. In-game results-state identity and a route showing actual native
results execution remain pending; see the [current evidence and limits](../docs/evidence/battle-results-batch3-20261002.md).

> A native port requires every executed function to be real C — raw `INCLUDE_ASM`
> (MIPS) cannot run on x86. Port progress is therefore gated on functional
> decompilation coverage; byte-for-byte matching is **not** required for the port.

## The Phase 1 loop (boot-path-driven decompilation)

The port doubles as the decompilation priority oracle:

1. Compile game TUs in port mode (`-DXENO_PC_PORT -DSKIP_ASM`, against the
   `include_shim/` -> PsyCross headers). `INCLUDE_ASM` becomes inert.
2. Collect undefined symbols: `nm -u *.o`.
3. `tools/scripts/gen_port_stubs.py` classifies each (function vs data, via the
   matching-build ELF symbol tables) and emits logging stubs + zeroed storage.
4. Link the executable and run it. It runs real decompiled code until it hits a
   stub on the live path, which logs `[stub] <name>`.
5. Decompile that function (matching optional), rebuild, repeat. Each step
   extends how far the game runs — and grows the matching % at the same time.

Reproduce the proven slice (Linux, in a container/distrobox with `gcc`,
`libc6-dev`, `binutils`, `python3`):

```bash
# from repo root; build the matching ELFs first (for symbol classification)
#   (in the ethos container)  make -B build

gcc -c src/field/game_logic/gold.c -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
    -include assert.h -w \
    -I pc_port/include_shim -I include \
    -I pc_port/extern/PsyCross/include -I pc_port/extern/PsyCross/include/psx \
    -o gold.o
nm -u gold.o | awk '{print $2}' | sort -u > undef.txt
python3 tools/scripts/gen_port_stubs.py \
    --elf build/out/slus_006.64.elf --elf build/out/field.elf \
    --undefined undef.txt --out pc_port/generated/stubs.c
```

## Game data and Sony code

The port ships no game data and no Sony Psy-Q code.

- **Game data comes from your disc at run time.** Put your Xenogears (USA,
  SLUS-00664) files in `disc/` (git-ignored): the Disc 1 image
  (`disc1.bin` + `disc1.cue`, or `XENO_DISC=`) and the files the matching build
  extracts (`SLUS_006.64`, `field.bin`, `menu.bin`, `shop_menu.bin`,
  `member_change_menu.bin`, `world_map.bin`; or `XENO_DATA_DIR=`). Native
  tables in `src/data_*.c` are zero storage plus (file, offset, size) rows;
  `src/retail_data.h` fills them before `main()` after checking each file
  against `config/checksum.sha`. On first run `src/disc_check.c` checks the
  disc image against the Redump dump (redump.org/disc/177) and remembers the
  result in `~/.cache/xenogears-port/`. A missing or wrong file stops the
  port with an explanation.
- **No retail bytes in git.** `tools/analysis/retail_data_guard.py` fails when
  a tracked file carries retail bytes (compared with your `disc/` files, or
  with committed one-way hash signatures in CI).
- **No Psy-Q in the port.** The PSX layer is PsyCross (MIT, pinned; see
  `THIRD_PARTY.md`) plus `src/psyq_compat*.c`. `tools/analysis/psyq_port_guard.py`
  fails on Psy-Q functions/tables in `pc_port/`, and `build_port.sh` runs it
  on the linked binary so no matching-build Psy-Q TU can be linked.
- Run all checks with `make port-guards`; install
  `tools/analysis/pre-commit-port-guards` as a git pre-commit hook (or use
  `.pre-commit-config.yaml`). CI runs them in `.github/workflows/port-guards.yaml`.

## Port layers, config and mods

Full description: [`docs/port/ARCHITECTURE.md`](../docs/port/ARCHITECTURE.md).

Port code reaches the platform through interface headers in
`include/xg_plat/` (the moddable-port pattern of the Parasite Eve port's
`docs/ARCHITECTURE-PORT.md`), each with a swappable backend in `src/plat/`:

| Interface | Covers | Backend now |
|---|---|---|
| `disc.h` | disc image, retail files, BIOS, sector reads, verification | stdio + `retail_data.h` + `disc_check.c` (weak, overridable) |
| `renderer.h` | window/output size, fullscreen, vsync, filtering, widescreen query | PsyCross |
| `input.h` | keyboard bindings by SDL key name | PsyCross |
| `timing.h` | game speed, fast-forward, vblank clock | PsyCross |
| `audio.h` | user volume | none yet (reported unsupported) |
| `config.h` | `config.ini`, `--config`/`--set`, `XENO_*` overrides | `src/plat/xg_plat_config.c` |
| `mods.h` | `mods/` folders, events, C plugins, asset replacement | `src/plat/xg_plat_mods.c` |

The game's own PSX calls still go through PsyCross + `psyq_compat*`; areas
move behind the interfaces as they are touched.

**Config:** copy `config.example.ini` to `config.ini` next to the executable or
to `~/.config/xenogears-port/`, or pass `--config FILE`; `--set key=value` and
`XENO_<SECTION>_<KEY>` override single keys. Widescreen, 60 fps and user volume are accepted but
reported as unsupported until a backend implements them.

**Mods:** each folder in `mods/` (or `~/.local/share/xenogears-port/mods/`)
has a `mod.txt` (`name`, `version`, `load_order`, `abi = 1`), optional
`assets/<sha256>.bin` replacements keyed by the SHA-256 of the original bytes
(archive files, streamed files, `LoadImage` texture blocks, `SpuWrite` sample
blocks), and an optional `plugin.so` exporting `xg_mod_init()`. Events: `boot`,
`frame_tick`, `asset_loaded`, `room_enter`. `XENO_MODS_LOG_ASSETS=1` prints the hash of each
file the game loads; `XENO_MODS_DUMP_DIR=dir` saves the originals from your
disc for editing. Mods are user content: `mods/` is git-ignored and must never
be committed. Full guide: [docs/port/MODDING.md](../docs/port/MODDING.md).

**Cheats:** `god`, `encounters`, `save`/`load`, `warp`, `battle`, `speed` are
cheat-console commands (`src/cheat_console.c`) run on `frame_tick`; the host
toolbar and F10/F12 queue them, and `cheats.console = on` reads them from stdin.

## Building (Linux)

For development toward an interpreter-free game, run from the repository root:

```bash
XENO_NATIVE_ONLY=1 ./pc_port/build_port.sh
./pc_port/build_native_only/xeno-port
```

This separate target excludes the CPU instruction decoder and executor at
compile time. It stops at an unconverted CPU call even if
`XENO_DISABLE_CPU_INTERPRETER=0` is set. It retains native rendering/audio
compatibility code and the high-level field-script VM. It is an incomplete
conversion target: missing gameplay bodies and generated stubs still require
work. The ordinary `pc_port/build_native` output is preserved. See the
[milestone and verification limits](../docs/port/NATIVE_CONVERSION.md).

The port runs from the game discs without a Sony BIOS. Memory-card titles and
field glyph strips use the bundled public-domain Jiskan 16 bitmap font, mapped
from Shift-JIS codes to the same 16x16 KROM glyph shape. Its pixels can differ
from Sony's font; details and source are in `font_data/README.md`. Setting
`XENO_BIOS` explicitly opts into the SCPH-5500 KROM path, which requires the
user's 512 KB BIOS with SHA-256
`11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef`.
The verified copy is retained for the process lifetime. An explicitly chosen
but invalid BIOS stops with `KROM_UNRESOLVED`.

Requires `cmake`, a C/C++ compiler, and dev packages for **SDL2**, **OpenAL**,
and **OpenGL**. The full `build_port.sh` driver also requires **OpenSSL 3**
development files (`libssl-dev` on Ubuntu) for the optional `XENO_BIOS` path.
The BIOS payload is not embedded in the build. On the immutable Bazzite host, build inside a container or a
`distrobox` (Ubuntu shown):

```bash
# from repo root
# PsyCross is pinned in pc_port/psycross.lock (MIT, see pc_port/THIRD_PARTY.md);
# pc_port/build_port.sh clones it at that commit when absent. Manually:
git clone https://github.com/OpenDriver2/PsyCross.git pc_port/extern/PsyCross
git -C pc_port/extern/PsyCross checkout e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6

cd pc_port
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
./build/xeno-port      # opens a window on a real display
```

## Host hotkeys

- A clickable host toolbar is reserved above the game viewport. Its `SAVE`,
  `LOAD`, and `RECORD` buttons perform the same actions as the hotkeys below.
  Save/load displays amber `WAIT` while field control is locked, green
  `SAVED`/`LOADED` on success, or red `ERROR` on failure. While recording,
  `RECORD` becomes a red `STOP` button. The toolbar is drawn after capture
  readback and is therefore not included in screenshots or MP4s.
- `F7` writes a field checkpoint to `quicksaves/quick.xgqs`.
- `F8` validates and loads that checkpoint, including its field, entrance, game
  state, and player position. Save/load requests made during dialogue,
  transitions, scripted control, or a menu are queued until the next safe field
  frame because this is a field checkpoint rather than a full process savestate.
- `F9` starts or stops an MP4 recording in `recordings/`. Linux recording
  requires `ffmpeg` and `pactl`; it records the completed game framebuffer as
  H.264 and the default PipeWire/Pulse output monitor as AAC stereo. Because the
  monitor is the system output, other desktop sounds are recorded too.

Set `XENO_QUICKSAVE_PATH`, `XENO_RECORDING_DIR`, or
`XENO_RECORDING_AUDIO_SOURCE` to override the checkpoint file, recording
directory, or Pulse monitor source respectively.

### Linux portability notes (handled in `CMakeLists.txt`, no vendored edits)
- PsyCross's bundled CMake is bypassed (case-sensitive globs miss its uppercase
  `*.C` PSX files; it expects SDL2/OpenAL as subprojects). We compile its sources
  directly and use system SDL2/OpenAL/OpenGL.
- `src/port_compat.h` is force-included to supply `strcasecmp`/`strncasecmp`
  (PsyCross's `include/psx/strings.h` shadows the system header).
- `-fpermissive` downgrades 64-bit function-pointer-to-int casts in the PSX SDK
  code (Phase 1 will revisit any that land on the boot path).

## Files
- `CMakeLists.txt` — native build: compiles PsyCross + the port executable.
- `src/port_main.c` — entry point; brings up PsyCross (Phase 1 hands off to the game).
- `src/xeno_pc.h` — port build identity + `INCLUDE_ASM` neutralisation plumbing.
- `src/port_compat.h` — Linux/gcc compat shim for PsyCross.
- `extern/PsyCross/` — vendored HAL (gitignored).
