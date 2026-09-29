# Port audit against the matched decomp C (2026-09-28)

Purpose: preservation. The port must run retail logic 1:1 ([../ARCHITECTURE-PORT.md](../ARCHITECTURE-PORT.md)).
This audit reads every function on the played paths that has byte-exact
matched C, and records what the port runs instead and where it diverges. It is a
static read of each port body next to its matched C (`src/`). Runtime evidence is
in "Behaviour checks" below. Per-function tables (local, git-ignored):
`~/.cache/xeno/{slus,field,menu}/audit_*.tsv`. Open divergences are listed in
[RETAIL_DIVERGENCES.md](RETAIL_DIVERGENCES.md) and in
[ARCHITECTURE-PORT.md](../ARCHITECTURE-PORT.md#known-divergences-public-list).

Auditors: main exe and world map (match-slus); field, scripts, battle and movies
(match-field); menus, items, equipment, shop and member change (match-menu);
aggregation, classifier, coverage and oracle (port).

## Port status of matched-C functions

Status per function:
- **matched C**: runs the byte-exact source.
- **faithful adapter**: a port body that reproduces the matched C with host
  seams only (layout, pointer width, instrumentation).
- **hand body**: a translation that has not been verified against the matched C.
- **partial cut**: drops part of the retail behaviour; what is dropped is named below.
- **absent**: no port body; a call to it is unreachable or stops the path.

| System | matched C | faithful adapter | hand body | partial cut | stub | absent | retail interpreter |
|---|---:|---:|---:|---:|---:|---:|---:|
| main exe (system, audio, CD/archive, heap, input, save plumbing) | 498 | 64 | 1 | 10 | 0 | 6 | 0 |
| world map | 2 | 89 | 1 | 13 | 0 | 14 | 0 |
| field (walking, actors, rendering, script VM) | 617 | 11 | 3 | 0 | 0 | 0 | 0 |
| battle | 95 native leaves | 9 | 7 | 0 | 18 | 0 | 228 |
| movies | 5 | 1 | 3 | 0 | 0 | 9 (CD-tool helpers, unreached) | 0 |
| menu / shop / member change | 321 | 10 | 16 | 0 | 0 | 0 | 0 |

The battle stub and interpreter counts overlap: 18 matched-C functions are
still represented by generated logging stubs at the native link boundary, and
their retail bodies remain available to the MIPS interpreter. The stub count
is also listed in the shadow TSV. Battle counts include the five shared bodies
verified through commit `b9aa1116`; the route itself remains untested. Menu
counts reflect the later menu audit export (2026-09-29). Status exports used
for this table are local-only under `~/.cache/xeno/{slus,field,menu}/audit_*.tsv`.

Field also runs 124 hand bodies for functions that have no matched C yet;
they are counted with the ratio in ARCHITECTURE-PORT.md, not here.

## Confirmed divergences

Fixed in this audit (claude/port, verified by route and test runs; see the commits):

1. **World-map exits could send the field to the wrong place.** Ten mode
   teardowns (`func_80077480`, `80077CC0`, `80078D24`, `8007A8AC`, `8007C260`,
   `80080218`, `8008106C`, `800826B4`, `800837DC`, and the mode-15 teardown)
   wrote the destination map, entrance and heading to guest RAM only. FieldMain
   reads the host `g_GameState` copy. Fixed: one store for both copies
   (`world_map_gamestate.h`).
2. **World position was never saved.** `func_8008E034` / `func_8008DFF4` kept
   the world position only in the guest copy, so saves and checkpoints missed it.
   Fixed: the save path stores both copies, and the restore reads the host copy.
3. **A second world-map session in one visit exited the process.**
   `func_8009766C` refused a pool pointer left set by the previous session
   (retail allocates unconditionally), and the setup stages (`func_80072090`
   and others) refused a second run. Fixed: no refusal; stage guards reset at
   each session.
4. **Battle stopped in DrawOTag when main-exe data zeroed outside the repo root (D5).**
   `PsxMemory_LoadStaticData` searched only `XENO_SLUS` and cwd-relative paths.
   Runs from another directory (an installed game) with `XENO_DATA_DIR` got
   zero-filled .rodata/.sdata. Battle's OT sentinel 0x8005698C read 0 instead
   of tag 0x04FFFFFF, and the first battle stopped after about 694,700
   instructions. Fixed: the disc layer's resolver, now also exe-relative
   (`run_static_data_cwd_test.sh` and `run_static_data_package_test.sh`).
5. **Title attract / new game was unreachable headless.** It was a test-harness
   bug, not a game divergence: `XENO_PAD_TEST_INPUT` pressed physical buttons
   while documented as game buttons. Under the retail USA layout the Circle
   press reached the game as Cross. The current run logged the ordered title,
   new-game, prologue and Lahan markers with no fatal marker; its wrapper ended
   with status 143 before writing the final summary, so the complete smoke
   verdict is unverified.

Open, by system (evidence in the tables):

The fixes below are described by subject as well as by the public D identifiers.
D5 is the executable-relative static-data loader regression and D6 is the
title test-harness button mapping; both fixes are supported by the regression
and route evidence above.

World map / main exe:
6. World-map story-selector callbacks (brackets 3–8): were **absent**, so
   the scheduler stopped at their slots. Switched to the matched C in
   batches 2–3 (17 callbacks). `func_80088720` runs a stopgap
   transcription until it matches. No current route reaches these brackets.
   Batch 4 (4ea5cab0) switched the seven mode-12 initializers
   (`func_8007CC6C`..`func_8007D774`) to the matched C; the retired
   `wm_8007CC6C` had an extra store (slot +0x20) retail does not make.
6a. `func_8008E76C` (player callback, not matched yet): states 8/12/16/20
   run `world_map_state_vertical_8f690.c` (1f4d64f5), a **hand translation
   from the retail assembly**, not matched C, like the restored states 0-2.
   It stays a hand body in this audit and in the shadow counts until
   `func_8008E76C` matches; then the port switches to that C and the module
   retires. `wm_800767D4` in the same file is a port body of the matched
   `func_800767D4` (guest-address arguments), counted as a faithful adapter.
7. `func_80087734`: the port calls `RotMatrixYXZ`, where retail calls
   0x8003F738 (named `RotMatrix`, a table-driven routine). Unconfirmed: needs a
   numeric check of 0x8003F738's rotation order.
8. `func_8001AADC` drops the clear of guest word 0x80062520.
9. `func_8001D468` adds a store retail doesn't have: it clears the NPC
   palette index (sprite+0x3C bits 20–23) before each `func_8001DAE8`.
10. `ArchiveReadFile`: stream mode 0x200 (2340-byte XA ADPCM sectors) is refused
   (XA audio not played), and several state stores are dropped.
   `func_8002A2D0` is replaced by `ArchiveCdSeekOrPause` at the field call site.
11. Sound: `func_800399D4`, `func_8003A89C` return early on a NULL or
    non-canonical manager. `func_80039C4C` / `func_80039C8C` skip
    `SoundHandleError(5)`. `func_8003E44C` skips the element store and
    `func_8003E5BC` when no WDS entry exists.
12. `func_8002C644` skips `HeapInsertAlloc` for out-of-range blocks (host
    guard) and prints on every pin.
13. `func_8003611C` skips the BIOS pad actuator-buffer registration
    (no actuator on the host).
14. The port's PSX scratchpad is split: world-map code maps 0x1F80xxxx
    through `PSX_ADDR` (a RAM mirror), while battle and some field code use
    `g_PsxScratchpad`. It is consistent within each subsystem, but not
    retail-shaped. A port-wide `PSX_ADDR` scratchpad mapping needs the 75
    standalone tests that define only `g_PsxRam` updated.
15. g_GameState has two copies (host blob, guest image). World-map exits and
    the saved position are now synchronised (fixed item 1–2), but matched
    world-map bodies write only the host copy, so hand-written world-map code
    that reads the guest copy of the same field sees it stale. Unifying the
    two (host `g_GameState` as an alias into `g_PsxRam` at 0x6D634) is the
    retail-shaped fix.

Field / battle / movies:
16. `func_800ACB90`: TIM uploaded at VRAM y 0 instead of 0x100.
17. `func_800799D4`: the field-menu overlay buffer is sized from the decoded
    file; retail's fixed-map-gap branch (D_8004F370 != 1) is dropped
    (host heap adaptation).
18. `FieldGetVec1Magnitude` zeroes vy/vz that retail leaves uninitialised.
19. Battle `func_800A3490`, `800A3514`, `800A3578` and `800A35C8` now compile
    one shared matched C body apiece, with the port's signed 16-bit callback
    returns preserved at the runtime bridge. The matching build remains
    byte-exact (`rom-check` 10/10 after the change). Their battle-route behavior
    is still untested. Follow-up Codex batches also moved `func_8009E53C`,
    `func_8009E3C8`, `func_800BED30` and `func_800A429C` to shared bodies with
    host-only pointer seams; their instruction sequences match retail
    (35/35, 18/18, 7/7 and 43/43 words), and focused guest-RAM tests pass at
    O0/O2/UBSan. These battle-route behaviors remain untested. Other adopted
    battle bodies and override `func_800BA8F4` remain unreviewed; see the
    current shadow list for bodies still compiled only under the port path.

Menus:
20. Port-only NULL guards in `func_801CE660`, `801D22C4`, `801D2968`,
    `801C8BEC`, `801C8EE8`, `801D3B00` (being collapsed to retail).
21. `func_801C81E0` adds a divide-by-zero guard.
22. `func_801E76EC` calls `func_801E733C` without retail's entry argument.
23. `func_801DBDB4` initialises a value retail leaves uninitialised.
24. `ShopMenuBuyMenu`: retail writes 8 flags into a 4-byte stack slot (a PS1
    overrun); the port enlarges the slot. Possible divergence, unconfirmed.
25. Shop's retail debug `break 0x400` is omitted.
26. `func_801CD710`: the handler result is not used (handlers have no port body).

## Behaviour checks (numbers, not reading)

- Title → new game → prologue → Lahan (map 2): the current smoke log contains
  all ordered route markers, and no fatal marker. Its wrapper ended with
  status 143 before writing a final summary, so the route is observed but the
  overall smoke verdict is unverified. This supports the harness button-bit
  fix; it does not establish that every title attract input route matches
  retail.
- World map: on-foot walk onto the Lahan trigger, natural exit, FieldMain map 1
  (`run_w34n124_world_walk_entry.sh`). It passes with the batch-1 matched bodies.
- Streamed sound bank: served from a mod on field map 1, run identical to the
  unmodded one (`run_stream_replacement_live_test.sh`).
- Retail oracle (PCSX-Redux + OpenBIOS, local only): see "Oracle".

## Oracle

PCSX-Redux headless interpreter, FREE OpenBIOS, owner-provided Disc 1 BIN/CUE;
outputs are local under `~/.cache/xeno/oracle/` and are not committed. BIOS
SHA-256: `713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d`.
The Sony BIOS present in the local disc directory was not used. PCSX-Redux's
`-iso` input here accepts ISO or BIN/CUE; passing the adjacent CHD left OpenBIOS
on its cube demo. Re-running with `disc1.cue` booted the game.

Captured retail states at VSync 3,600 (Xenogears title logo), 4,600 (title
menu), and 9,000, 11,000, and 13,500 (title save/load screen with “No Memory
Card”). The latter state persisted through the final capture. This confirms
boot and title/menu presentation only. The scripted button schedule did not
advance into the new-game prologue, and these captures have not been compared
pixel-for-pixel with the port. Field, battle, and menu route comparisons remain
untested. Raw frames and input/event logs are local in
`~/.cache/xeno/oracle/codex_20260929_route/`; no retail data or oracle output is
in Git.

## Fix order

1. Battle hand bodies (item 19). The static-data fix removes the first-battle
   stop, and the bounded CWD regression confirms battle continues for 35
   seconds. Extend battle route assertions and test each switched body there.
2. Audio: XA sectors and the sound partial cuts (items 10, 11).
3. g_GameState and scratchpad unification (items 14, 15).
4. Remaining partial cuts (7–9, 12, 16–18) and menu hand bodies (20–26), in
   route-tested batches that switch each to its matched C.
5. The rest of the shadow list (docs/port/shadowing_matched_c.tsv), batch by batch.
