# Field Script VM and Opcodes

Decoded field script VM behavior from committed handoff notes and source references. Noah is cited only as readability corroboration — retail asm and runtime traces are authoritative.

**Two interpreters live on this page.** The sections down through "Opcode 7 decode example" cover the **field script VM** (per-map bytecode, `FieldScriptVMRun`). The July 9, 2026 reload campaign decoded a second, separate interpreter — the **sprite animation-script VM** — documented in its own section below. Opcode numbers are NOT shared between the two.

## Field VM dispatch (decoded / trusted)

- Primary handler table: `g_FieldScriptVMHandlers` — opcodes 0–255
- Extended table: `g_FieldScriptVMHandlers2 = &g_FieldScriptVMHandlers[256]` — opcodes 256–482 (483 total handlers)
- Byte **`254`** (`0xFE`) = `FieldScriptVM2Run` — prefixes extended opcode
- Dispatch: `src/field/scripts/virtual_machine.c` (handoff cites line 250)
- Full handler name list in `pc_port/src/data_field.c`

## Key opcodes (documented)

| Opcode | Name / handler | Role | Status |
|--------|----------------|------|--------|
| `0xA7` (167) | `func_8009F5F4` — `OP_UPDATE_CHARACTER` | Per-frame player update; `scriptFlags & 0x4000` branches walk vs idle | **Committed** `e7b0101` |
| `7` | `func_8009EB78` | Start actor script routine; scans 8 slots for free `(priority==0xF, busy==0)` | **Decoded**; slot-init fix `9e1b667` |
| `54` | `FieldScriptVMHandlerVariableSetTrue` | Set script variable true | **Decoded** |
| `55` | `FieldScriptVMHandlerVariableSetFalse` | Set script variable false | **Decoded** |
| `116` (`0x74`) | `func_8008F668` | Set warp dest; calls sound cue path | **Reached** in exit trace |
| `203` | `FieldScriptCheckTriggerZone` | 3D height-aware trigger poll | **Proven** zones 8–11 |
| `254` + ext | `FieldScriptVM2Run` | Extended opcode dispatch | **Decoded** |
| `0xA8` (168) | `FieldScriptVMHandlerMulVariableWithRand` | `var = (rand() * (arg+1)) >> 15` | **Decoded**; PSX rand range issue on PC |
| `0x5F` (95) | `func_8009AD6C` | Actor direction handler | **Committed** `2627286` |
| `0x23` (35) | HideActor | Hides actor (actor 18 init script) | **Observed** |
| `179` / `180` | FadeOut / FadeIn | Transition fades | **Not reached** on current exit route |
| `152` / `71` / `234` | Map-load family via `func_80092894` | Writes `g_GameSceneMapNum`, entrance, `D_800ADBEC` | Op 152 (`func_800932D0`, CHANGE_FIELD) **Committed fix** `fe66d9e`, verified on zone-5 reload; 71/234 still gated |

## Map1 trigger systems (proven)

### Encounter zones (0–7) — 2D

- Opcodes at IPs 6152–6176 poll zones 5, 2, 7, 1, 0, 4, 6
- Handler: `FieldScriptHandleTriggerZone2D` (`misc11.c:844`)
- Inside path: `misc11.c:873–874` → subroutine → **`func_80093B10`** enables random encounters
- **All 7 zones are encounter-region activators**, not map transitions (July 7 reading; superseded — the zone-5 trigger path drives the A50 script chain which issues CHANGE_FIELD, commits afb5535/fe66d9e/3c8f954)
- Verified: zone 5 fires with Fei at documented position (`94d8cfc`)

### Exit zones (8–11) — 3D height-gated

- Opcode **203** polls zones 8/9/10/11 at IPs 5991/6018/6045/6072
- Inside path: **`misc11.c:992`** (`FieldScriptCheckTriggerZone`)
- Armed when script vars `1116/1118/1120/1122 == 0`
- Transition bytecode after inside: **op7** + **op116** + **op54** + jump + clear
- Via the zone-5 reload probe — not the zones 8–11 door bytecode: full transition chain now **Verified working** end-to-end (July 9, 2026): zone-5 CHANGE_FIELD (op 152, `func_800932D0`, `fe66d9e`) → `func_800A5C40` transition orchestrator (`4b377be`) → `FieldFree` heap reclaim (leak fixed `fe8933f`, verified `f48a4ad`) → `FieldLoad` #2 map15 @ frame 116 → load-time animation-script VM (section below) → first post-load frame 117. The natural exit path through zones 8–11 remains gated by the `rand()`/`var0x0408` issue.

## Exit route script decode (actor 48 routine 1)

Zone 11 path (documented static decode):

```
0x17b8: trigger zone 11 → target 0x17d0
0x17bc: if var0x0462 == 0 → continue else jump 0x17cd
0x17c4: op7  (actor 18, routine 4)
0x17c7: op116 (sound cue)
0x17ca: op54 set var0x0462
0x17cd: jump 0x17d3
0x17d0: op55 clear var0x0462
0x17d3: stop
```

**Var `0x0462`** is a **local latch** preventing repeated op7 — not a fade consumer (`e13c0e3`).

## Actor 18 script state (decoded)

| Routine | Offset | Content |
|---------|--------|---------|
| 0 | `0x069f` | Sprite init (`0xBC`) + stop |
| 1 | `0x06a1` | State machine on **`var0x0408`** |
| 4 | `0x07cf` | Short Y-rotation door animation (no fade/map-load) |

Actor 18 routine 1 branches:

- `var0x0408 == 0` → `0x06af`
- `var0x0408 == 1` → sleep/stop at `0x0710` (**current path**)
- `var0x0408 == 2` → rotation branch `0x071e`
- else → fallback `0x0777`

## Actor 20 — owner of `var0x0408` (July 8 frontier, superseded)

- Routine 1 at `0x0888` uses `0xA8` random chooser (max 4) then dispatches states 0–4
- Fallback at `0x0997` assigns `var0x0408 = 1`
- **PC port bug:** host `rand()` range breaks chooser → always falls back to `1` (`8db4fb2`)

## Actor script slots (decoded / committed)

- 8 slots per actor; base `p + 0x8C + i*8`
- Idle/free flag word: `0x003cffff` (priority `0xF`, busy clear)
- **Bug:** `func_80080A74` used wrong slot base → busy bit set on idle slots
- **Fix:** `9e1b667` mirrors retail writes

## Opcode 7 decode example (zone 11)

After trigger, bytes at IP 6084: `07 12 24`

- Target actor selector `0x12` → actor **18**
- Routine byte `0x24` → script/routine **4**
- Priority **1**

## Sprite animation-script VM — second interpreter (July 9, 2026)

**This is NOT the field script VM.** Every field sprite runs its own animation script: dispatcher **`func_800248D4`** (`src/slus_006.64/system/temp1.c`) executes base opcodes and advances the script pc via the per-opcode stride table **`D_8004FC40[256]`**; opcodes `>= 0x8A` go through the extended-opcode handler **`func_8001FBE4`** (`src/slus_006.64/system/animation_scripts.c`) via `jtbl_800183D8[opcode - 0x8A]`. The Map1→Map15 reload hit it when map15's load-time field script started ticking sprites (`FieldScriptVMRun` → `func_800A1624` → `func_80076AC0` → `AnimScriptTick` → `func_800248D4`). Depth: the July 9 entries in `docs/ai_context/ACTIVE_HANDOFF.md`.

### Passive unimplemented-opcode survey (July 13, 2026)

The dedicated unimplemented set is `0x85, 0x8E, 0x98, 0xBE, 0xC8, 0xD4,
0xE2, 0xFA`; `0x86` is already implemented and was a stale report. A
compile-time `XENO_DIAG_OPCODE_SWEEP` hook emits fixed binary records to a
launcher-supplied fd-3 file. Records carry opcode, sprite-data address,
animation-script PC, matched field actor index where available, and animation
index. The launcher appends `END!` after its watchdog returns.

Passive starts observed 16 dispatches on Map000, 195 on Map001, and 48 on
Map014; none used the unimplemented set. This is **observed startup
coverage only**, not evidence that the opcodes are unused. Targeted progression
coverage is required before choosing an implementation order.

### Static reachable-opcode survey (July 14, 2026)

`tools/scripts/psx/scan_field_anim_opcodes.py` corrects a premise that must stay
explicit: actor routine offsets in the field-script archive belong to
`FieldScriptVMRun`; they are **not** entry points for `func_800248D4`. The
animation dispatcher consumes animation records inside each map's decompressed
sprite-data section, so the scanner seeds traversal from those real animation
entries instead.

The scanner derives generic strides from retail `D_8004FC40`, then applies the
retail dedicated-handler semantics and fixed-size overrides. In particular,
`D_8004FC40[0xBE]` is 2, but the dedicated handler at
`0x80024A84-0x80024B9C` consumes three bytes. Blindly treating the table as the
whole decoder would desynchronize at every `0xBE`. No opcode is variable
length. Relative jumps, conditional branches, `0xE2` calls, and `0x85` returns
are followed with a visited `(PC, call stack)` set and strict package-code
bounds. An invalid fetch/target, zero advance, or truncated instruction aborts
that animation and excludes all of its partial counts.

Full-disc result: **730 maps, 3,234 sprite packages, 16,382 animation entries,
zero aborts, zero map/package errors**. Only `0xBE` is reachable among the eight
dedicated unimplemented opcodes:

| Opcode | Reachable instruction sites | Raw unreachable/operand bytes | Maps |
|--------|-----------------------------:|------------------------------:|------|
| `0x85` | 0 | 268 | — |
| `0x8E` | 0 | 852 | — |
| `0x98` | 0 | 1,194 | — |
| `0xBE` | **7** | 261 | 47 (3), 48 (1), 334 (3) |
| `0xC8` | 0 | 1,157 | — |
| `0xD4` | 0 | 3,131 | — |
| `0xE2` | 0 | 2,144 | — |
| `0xFA` | 0 | 235 | — |

The raw-byte column is deliberately non-authoritative: it includes operands and
unreachable/trailing bytes. For example, a byte grep would promote `0xD4` based
on 3,131 matches even though none is a reachable instruction.

Implementation priority therefore collapses to `0xBE`, with Map047 package 0
as the first runtime binding/repro target (Map334 ties its concentration).
Package/animation entries are statically known, but actor binding is dynamic
and must be identified at runtime. The seven zero-count opcodes are deferred,
not closed: party, battle, and special-animation packages live outside the
per-map sprite-data coverage of this scan.

### Cleared anim opcodes (asm-faithful, adversarially verified vs retail asm)

| Opcode (ext idx) | Semantics | Status |
|------------------|-----------|--------|
| `0xC6` (`0x3C`) | Conditional operand store to `*(+0x7C)+0xC` | **Committed fix** `ce61f3d` |
| `0x96` (`0x0C`) | Unlink owned work-list entries via `func_8001CE74` | **Committed fix** `ce61f3d` |
| `0xFC` (`0x72`) | Script-embedded VRAM image upload: 24-bit script-relative blob → `func_8001FB30` → `func_8002DDE4` multi-block `LoadImage` (pixel `0x1100` / CLUT `0x1101` blocks) | **Verified working** `ce61f3d` — blob live at VRAM 512,256 (`map1_opcode_fc_reload_20260709_run1.log`) |
| `0xE0` (`0x56`) | Child-sprite spawn via `func_80023B84` (subsystem below) | **Verified working** `ce61f3d` (`map1_opcode_e0_reload_20260709` logs) |
| `0x8D` (`0x03`) | Texture-page latch `func_8002CC10`: `D_80059310 = GetTPage(0,0,x,y) & 0x1F`; merge-override mode `D_80050108 = 1` | **Committed fix** `ce61f3d` |
| `0xF5` / `0xF6` (`0x6B`/`0x6C`) | Per-sprite model-data (re)load from a 24-bit script-relative blob under heap user 5: relocate → free old buffer → rebuild → fixup → mirror | **Committed fix** `11b2474` (`0xF7` left asserting by design) |
| `0xA3` (`0x19`) | Gravity setter → sprite`+0x1C` (consumed by the `func_80022B2C` integrator) | **Committed fix** `f24e035` |
| `0xBC` (`0x32`) | Multi-command position opcode; sub-dispatch below (subs `0x24`/`0x25` done) | **Committed fix** `de23142` / `ae30d53` |
| `0x94` (`0x0A`) | Mode 2: inherit parent's angle (`+0x32`) into transform rotation-Y + matrix-dirty flag | **Committed fix** `a389755` — **milestone: map15 load-time script VM completes** |

### `0xBC` sub-dispatch structure

- op0 **bit7 set** → sub-command dispatch through **`jtbl_800185A8`** (`0x27` entries, indexed by `op0 & 0x3F`); **bit7 clear** → anchor path (asserts, unported)
- op0 **bit6** picks the store target: target-position halfwords (`+0xA0/A2/A4`) vs live position words (`+0x0/4/8`)
- Sub `0x24`: set wrapper sticky bit (`unk14` bit 30 — detaches the entry from opcode-`0x96` bulk unlink); sub `0x25`: clear it — both **Committed fix**
- Sub `0x16` (move child to parent's position via the `+0x70` back-link; jtbl entry `0x8002044C`): **Decoded** — the current **Blocker**
- All other subs and the camera-relative `ApplyMatrixSV` tail assert loudly (bounded port)

### Child-sprite spawn subsystem (opcode `0xE0`)

- **`func_80023B84`** heap-allocates an `AnimTask` (timer work-list task; render work-list task at `+0x1C`; `SpriteData` at `+0x38`), clones ~40 parent fields/flag bits, binds the child script (`func_80023538`), runs per-type post-init `func_80024730`, and binds a render callback from retail table `D_8004FD40[16]` (`func_80025224`). Only slot `func_80025710` is ported; type-2 children need **`func_80025718`** — the queued child-visibility gap
- Tick callback `func_80022DF4` = `AnimScriptTick` + motion integrators (slow-motion scale, floor/bounce gravity, XZ motion); free callback `func_80022EB8`; ~16 functions in `pc_port/src/game_overrides.c` + work-list machinery in `pc_port/src/work_list_port.c` (`ce61f3d`)
- **Critical latent-bug fix** (`ce61f3d`): the port's `WorkListEntry` used native 64-bit pointers instead of the PSX `0x1C`-byte layout the game embeds in heap objects; the first real entries made `TimerWorkListUpdate` call a garbage callback. Fields are now `u32` — see [Matching-and-Porting-Rules](Matching-and-Porting-Rules)

### Runtime proof (zone-5 Map1→Map15 reload probe)

- Probe mechanism: the probe gdb-injects synthetic d-pad input (`D_800AFE9C = 0x2000`, +Z) at `misc2.c:1688`, gated to pre-reload frames; Fei walks into Map1's zone-5 trigger, whose script chain (the A50 script) issues CHANGE_FIELD (op 152) through the retail path — the transition itself is not injected
- **Verified working** `a389755`: map15's load-time script VM completes — 3 type-2 child sprites spawn, tick, and finish their scripts; `FieldLoad` returns and the first post-load frame (117) begins (`map1_opcode_94_reload_20260709` log, `captures/render_diag/`)
- Frame 117's per-frame child pump (`func_800752C8` → `TimerWorkListUpdate`, child ticks = 8) then reaches the `0xBC` sub-`0x16` **Blocker** (`map15_bc25_20260709_run1.log`)

## What is NOT decoded enough to implement blindly

- Battle overlay handoff (`func_80281204`, `LoadGameStateOverlay(2)`)
- Full encounter selection path in `func_80079288`
- Anim VM: `0xBC` bit7-clear anchor path, camera-relative `ApplyMatrixSV` tail, and the remaining sub-commands (assert loudly by design)
- Anim VM: opcode `0xF7` (retail reads an uninitialized-in-function `$s2`; left asserting by design)
- Anim VM: `D_8004FD40` render-callback slots beyond `func_80025710` — `func_80025718` (type-2 child visibility) is the queued next
