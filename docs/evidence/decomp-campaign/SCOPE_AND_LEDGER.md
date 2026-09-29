# Disc 1 matching campaign

## Scope

The concrete story boundary for this campaign is the opening through the
party's first arrival in Shevat. The build target inventory is a conservative
coverage envelope for that route: the main executable, field and battle
systems, menus, world map, and movie support that may be loaded along the way.
Runtime tracing has not yet proven that every configured overlay is reached
before this boundary, so no target is omitted on an unverified reachability
assumption.

The scope includes the main SLUS-006.64 executable and all nine overlay
artifacts listed in `gears.toml`: `field`, `member_change_menu`, `shop_menu`,
`menu`, `battle`, `battling`, `movie`, `battle_command_file1`, and `world_map`.
The field script/event VM lives in the executable and field overlay; it is not
a separate retail binary. The Psy-Q row in the dashboard is a subset of the
main executable and must not be added to the total a second time.

## How progress is measured

`make report` builds with `SKIP_ASM`; objdiff counts only C code that matches
the retail object bytes. Assembly-backed functions add zero matched C bytes.
`tools/scripts/progress_dashboard.py` generates [DASHBOARD.md](DASHBOARD.md)
with matched bytes and fully matched functions per target. The report is a
decompilation progress measure, not a whole-binary verdict.

`make rom-check` is the whole-binary gate. It rebuilds from clean and checks
each artifact listed in `config/checksum.sha` against its retail SHA-256. It
fails if a target is missing or differs. Hashes are never updated to make a
build pass. A target with checksum PASS can still contain assembly; PASS means
the produced retail module is byte-identical, not that every function is C.

rom-check, not objdiff, is the authority on whether a function matches.
objdiff can score a function 100% while its bytes differ: field's
func_800A7948 scored 100% even though two of its stores went to swapped
`symbol + offset` targets.

## Baseline — 2026-09-28

Source counts below come from objdiff report version 2 after `make report`.
The current report measures **287,540 / 1,383,684 code bytes (20.78%)** and
**1,916 / 4,464 functions (42.92%)** as matching C. The aggregate covers all
targets below; the Psy-Q subset overlaps the executable row.

| Target | Matching C functions | Function progress | Matching C bytes | Byte progress | Whole-binary SHA gate |
|---|---:|---:|---:|---:|---|
| Main executable (`slus_006.64`) | 813 / 1,223 | 66.48% | 95,180 / 218,072 | 43.65% | FAIL |
| Field overlay | 570 / 884 | 64.48% | 91,508 / 253,052 | 36.16% | FAIL |
| Member-change menu | 67 / 67 | 100.00% | 24,936 / 24,936 | 100.00% | PASS |
| Shop menu | 104 / 118 | 88.14% | 37,460 / 53,008 | 70.67% | PASS |
| Main menu | 70 / 307 | 22.80% | 10,784 / 148,360 | 7.27% | FAIL |
| Battle overlay | 292 / 857 | 34.07% | 27,672 / 332,316 | 8.33% | PASS |
| Battling overlay | 0 / 483 | 0.00% | 0 / 139,692 | 0.00% | PASS |
| Movie overlay | 0 / 34 | 0.00% | 0 / 26,972 | 0.00% | PASS |
| Battle command file 1 | 0 / 105 | 0.00% | 0 / 18,940 | 0.00% | PASS |
| World map overlay | 0 / 386 | 0.00% | 0 / 168,336 | 0.00% | PASS |

The checksum baseline is from a clean `make rom-check` after the current menu
assembly include was restored and `MenuExecute` reached a 100% objdiff match.
Its containing main-executable TU matches 8/8 functions and 2,816/2,816 code
bytes. `AnimScriptStackPopU8`, `AnimScriptStackPopU16`,
`AnimScriptStackPopU24`, `AnimScriptStackPushU8`, `AnimScriptStackPushU16`,
and `AnimScriptStackPushU24` are now exact C matches: these changes increased
main-executable progress by 284 bytes and six functions. Field C now includes
the exact 604-byte `func_80085C90`, exact 140-byte `func_8008CD48`, exact
88-byte `func_800ACC58`, exact 304-byte `func_8008E8C8`, exact 152-byte
`func_8009FE4C`, exact 120-byte `func_8009FDD4`, exact 964-byte
`FieldParticleRender`, exact 168-byte `func_80095CC4`, exact 220-byte
`func_80095D6C`, exact 784-byte `func_800A7948`, and exact 140-byte
`func_80086BA8`. The field particle TU is now 15/15 C functions.
**7 of 10** pinned artifacts pass. The three
failures are the executable, field overlay, and main menu; their hashes remain
red and must be repaired in code. `MenuExecute` is exact at the object level,
but the main executable as a whole remains checksum-red; more drift remains.

The dashboard initially omitted the world-map category even though objdiff
had 386 world-map functions in its unit list. `tools/objdiff/config.yaml` now
includes that category so future reports expose it explicitly.

## Priority

1. Main executable: repair the remaining checksum drift and convert its
   remaining assembly-backed functions to matching C in bounded batches.
2. Field overlay: repair its checksum drift, then address the large remaining
   unmatched C surface in the story and script/event paths.
3. Main menu: investigate the newly red hash and repair it before expanding
   the menu C surface.
4. Battle and the other overlays: prioritize functions reached by encounters
   and route transitions through the Shevat boundary.

Never mark a target complete on inferred C status alone. A target is byte
verified only when its `make rom-check` row passes against the unchanged retail
pin.

## Checkpoint — 2026-09-28 02:55 CDT (claude/integration @ 203a0780)

`make rom-check` from clean: **10 of 10** pinned artifacts pass
("ALL PINNED ARTIFACTS MATCH RETAIL"). Every target in scope is byte-exact
against its unchanged retail pin.

objdiff (`make report`) now measures **283,720 / 1,383,684 code bytes
(20.50%)** and **1,886 / 4,464 functions (42.25%)**. It is lower than the
baseline above because C that never byte-matched retail was returned to
`INCLUDE_ASM` for the retail build (the C stays under `#ifdef XENO_PC_PORT`).
objdiff is not the authority: it scored `func_800A7948` 100% although two
same-shaped relocations were stored in swapped order, and four main-executable
functions fail under `-DSKIP_ASM` although they are byte-exact in the real
link. Only the rom-check row decides whether a target is verified.

| Target | Matching C functions | Matching C bytes | Whole-binary SHA gate |
|---|---:|---:|---|
| Main executable (`slus_006.64`) | 722 / 1,223 | 85,256 / 218,072 | PASS |
| Field overlay | 592 / 884 | 93,776 / 253,052 | PASS |
| Member-change menu | 67 / 67 | 24,936 / 24,936 | PASS |
| Shop menu | 104 / 118 | 37,460 / 53,008 | PASS |
| Main menu | 109 / 307 | 14,620 / 148,360 | PASS |
| Battle overlay | 292 / 857 | 27,672 / 332,316 | PASS |
| Battling overlay | 0 / 483 | 0 / 139,692 | PASS |
| Movie overlay | 0 / 34 | 0 / 26,972 | PASS |
| Battle command file 1 | 0 / 105 | 0 / 18,940 | PASS |
| World map overlay | 0 / 386 | 0 / 168,336 | PASS |

Root causes of the three red rows:

- **Function order.** be7f3582 made `INCLUDE_ASM` a bare top-level `__asm__`.
  At `-O2 -G8`, cc1's gp-opt mode spools every C body after all file-scope asm,
  so assembly and C no longer interleaved in source order. The maspsx
  dummy-function wrapper is restored in `include/include_asm.h`.
- **C committed as matching that did not match:** about 240 main-executable
  functions, 308 field functions and 103 menu functions. Found with a
  relocation-masked per-function compare against the retail objects.
- **Main executable only.** PsyQ stubs had become `.word` blobs that dropped
  retail padding. `MenuExecute`'s jump tables were emitted twice. One real bug
  in `func_8001AADC`'s clear range.
- **Menu build model.** Retail menu never uses `$gp`, so it now builds with
  `-G0` (the `MenuG0` preset). Port-only helpers and edits had leaked into
  code the retail build compiles.
- **Main-executable build model.** Retail loads small constants with `addiu`
  in some TUs (the `XenoLiteral` preset), and reaches other TUs' globals
  absolutely rather than through `$gp`.

Behaviour differences found between the port and retail are listed in
[docs/port/RETAIL_DIVERGENCES.md](../../port/RETAIL_DIVERGENCES.md).
