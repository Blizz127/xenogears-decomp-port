# Decomp and PC-Port Coexistence Architecture

Surveyed at `fdd86e7` on 2026-07-14. This page records how the matching
decompilation and native PC runtime currently coexist, and which mechanism each
held subsystem should use. It is a design decision only; no subsystem was
activated by this survey.

## The linker rule

There is no link-order override convention. Game objects and port objects emit
ordinary strong symbols, so two definitions of the same name are a hard
multiple-definition error. A port definition only appears to "win" when the
matching implementation is absent from the native link: its TU is excluded,
its body is an inert `INCLUDE_ASM` under `SKIP_ASM`, or the symbol comes from a
raw overlay/data section rather than compiled C.

This is useful fail-loud behavior. A newly decompiled function that collides
with an old port copy cannot silently change ownership. The collision must be
resolved deliberately.

## Existing mechanisms

| Mechanism | Selection and granularity | Reference source | Tradeoffs |
|---|---|---|---|
| `SKIP_ASM` plus generated stubs | Every normal game TU is compiled. `INCLUDE_ASM` emits no native body, the trial link records the resulting undefined symbols, and `gen_port_stubs.py` uses matching ELFs to emit per-symbol function or data placeholders. | Matching C and MIPS ASM remain unchanged. | Most granular and the intended incremental-porting loop. Function stubs log once and return zero; they classify symbols safely but do **not** implement retail signatures or behavior. Data stubs have ELF/symbol-map-derived storage. A changed undefined set regenerates from ELFs or fails if only a stale manifest exists. |
| In-source `XENO_PC_PORT` branches | One compiled TU owns the symbol in both builds; small host-only operations, types, or accessors are selected by the preprocessor. Examples include packed-pointer access in field code, host OT clearing, and `func_8002C700`'s structural descriptor access. | Both implementations are co-located; the matching build continues to compile the retail branch. | Best for a bounded host representation difference inside otherwise shared logic. Both builds catch syntax/type drift, but semantic parity still requires tests. Broad conditional rewrites make matching source hard to read and should be avoided. |
| Whole-tree/TU exclusion plus replacement | `build_port.sh` omits a pattern or TU, then explicitly compiles a port source. `src/**/psyq/**` is replaced by PsyCross plus `psyq_compat.c`; `system/archive.c` is replaced by the partial synchronous `archive_port.c`. | Excluded sources still build in the matching graph and remain retail references. | Correct when the hardware/runtime model is fundamentally different. It permits a coherent host representation but duplicates code and requires manual synchronization. Missing exports fall through to generated stubs. Exclusions are printed at build time. |
| Manual strong per-symbol definitions | `game_overrides.c`, `psyq_compat.c`, and the `data_*.c` files define symbols that are otherwise undefined in the native link. `func_801E72CC`, child-sprite functions, boot plumbing, and migrated retail data use this form. | The matching C/ASM or raw overlay remains the oracle. | Useful for small, proven functions and raw-overlay symbols. It is not an override registry: if a real C definition later enters the native link, the build fails on the duplicate. The large `game_overrides.c` ownership surface is hard to audit and should not receive new subsystem-wide implementations. |
| Dedicated `_port.c` subsystem source | A port file supplies several related symbols using one host-safe model. `archive_port.c` and `work_list_port.c` are the current examples. | Original TU remains matchable and reference-only for the PC build. | Keeps host-only representation and helpers coherent. It works best when paired with a deliberate whole-TU exclusion; partial replacements otherwise create ownership gaps and duplicates. |
| Runtime descriptor/dispatch indirection | A port-owned table selects handlers at runtime. `D_8004FE50` routes model walkers, `D_8004FD40` routes animation render callbacks, and `PcPort_InitGameStates` constructs the host game-state table. | Retail tables and target ASM define the required entries. | Per-handler routing without linker collisions, but only where retail already has a dispatch boundary. Host pointer width can change table layout, so consumers must use structural access. Missing or wrong entries are not automatically detected; `D_8004FE50` required a complete retail audit. |
| Explicit held exclusion | `INTENTIONALLY_EXCLUDED_GAME_TUS` keeps a compiling TU out of both compile and link while a routing decision is unresolved. This is distinct from `KNOWN_BROKEN_GAME_TUS`, which tolerates a reviewed compile failure. | Source remains untouched and matchable. | Correct quarantine, not a runtime architecture. The build prints every hold and reason. A held TU must leave this list once its durable ownership rule is implemented. |
| Exact retail data migration | `data_field.c`, `data_font.c`, `data_kernel_menu.c`, and `data_published_logo.c` provide real initialized bytes instead of zeroed data stubs. | Retail ELF/data ASM remains authoritative. | Appropriate for immutable data with verified size/content. It is per-symbol and manual; broad use will not scale as well as loading/relocating complete retail sections. |

### PsyQ replacement is a compound mechanism

The `src/**/psyq/**` exclusion is a permanent whole-subsystem replacement:
PsyCross provides the hardware abstraction, `psyq_compat.c` supplies missing
game-facing helpers, and tracked patches under `pc_port/patches/` preserve
verified PsyCross fixes across regeneration. The matched PsyQ sources remain
valuable decomp/reference code but are not the live runtime.

### `game_overrides.c` is not one convention

It currently contains four different categories:

1. Native boot/heap/game-state plumbing with no directly runnable retail body.
2. Per-symbol implementations for inert `INCLUDE_ASM` or raw-overlay functions,
   including `func_801E72CC` and child-sprite paths.
3. Host-layout runtime tables and model walkers, including `D_8004FE50`.
4. Three sound definitions that collide when `sound.c` is linked:
   `SoundValidateFile`, `SoundFileComputeChecksum`, and `SoundAddSedsEntry`.

The first three are deliberate port ownership. The sound copies are historical
duplication, not evidence that `game_overrides.c` should own the sound
subsystem.

## Held-subsystem mapping and decisions

### Member-change and shop menus: compile normally, stub incrementally

The two menu TUs compile, have no `_port.c` replacement, and have no strong
symbol collision with the current port objects. Their missing native surface is
the ordinary `SKIP_ASM` case: 15 link-reachable functions (three member-change,
twelve shop) require placeholders in the activation trial. Matching overlay
ELFs now exist, so the normal stub generator can classify them.

**Decision:** remove the two menu TUs from the held-exclusion list in a dedicated
activation change. Compile their real C and let the exact undefined set produce
oracle stubs. Then drive real menu routes and replace each reached stub from
retail ASM. A zero-return function stub is only a diagnostic boundary, never
proof that the menu behavior works.

Whole-file replacement would throw away working decompiled menu code, and a
per-function override registry would add machinery where no collision exists.

### Work-list: keep a permanent host-layout replacement

The symbol comparison is exact: nine functions are strong definitions in both
`work_list.c` and `work_list_port.c`:

`WorkListsReset`, `TimerWorkListUpdate`, `WorkListUpdate`, the three callback
setters, `func_8001D298`, `func_8001D2B0`, and `func_8001D468`.

The matching TU also has five C-only exports absent from the port file, while
the port file supplies four globals and eleven functions whose matching bodies
are `INCLUDE_ASM`. Neither object is a complete drop-in replacement.

More importantly, the matching TU's `WorkListEntry` uses native pointer fields.
On LP64 that layout is 0x30 bytes with fields at 0/8/0x10/0x18/0x28; runtime
objects embed the retail 0x1C-byte layout with 32-bit pointer slots. The port's
packed-u32 representation has already been live-validated and is the required
runtime model.

**Decision:** treat `work_list.c` like the PsyQ/archive sources for PC runtime:
retain it as matching/reference code, keep the TU excluded, and make
`work_list_port.c` the canonical native owner of the complete subsystem. Port
remaining required exports into that file using its single packed layout;
unreached missing exports may remain oracle stubs. Exclusion alone is not
completion—the decomp-only getters/delete helpers and missing allocation/free
paths still need host-safe implementations when reached.

A future cleanup may move the packed type/accessors into a shared header and
co-locate conditional bodies in the matching TU, but that is code organization,
not an open runtime-layout decision.

### Sound: retain the decomp TU, fix its host boundary, then activate

`sound.c` contributes 136 native definitions in a port-mode object and exposes
39 link-reachable `INCLUDE_ASM` functions in the prior activation trial. Only
three definitions collide, all with historical copies in `game_overrides.c`:
`SoundValidateFile`, `SoundFileComputeChecksum`, and `SoundAddSedsEntry`.

The collision is not the only blocker. The survey found a broader LP64 layout
hazard in `include/system/sound.h`. Comments describe retail offsets, but native
pointers widen them—for example:

| Type/field | Retail comment | Current LP64 offset |
|---|---:|---:|
| `SoundFile.pNext` | `0x1C` | `0x20` |
| `SoundWDSEntry.pNext` | `0x2C` | `0x30` |
| `SoundHeapBlockHeader.pNext` | `0x0C` | `0x10` |
| `SoundTransferCommand.pSpuData` | `0x04` | `0x08` |
| `SoundTransferCommand.pCallbackFn` | `0x10` | `0x20` |
| `AudioManager.unk_Manager_0x4` | `0x04` | `0x08` |

This code handles retail file data, packed queues, linked lists, callbacks, and
host SPU interfaces, so blindly linking the TU would repeat the work-list class
of defect across a larger subsystem.

**Decision:** do not create a whole `sound_port.c` rewrite and do not grow the
sound section of `game_overrides.c`. First audit each pointer-bearing sound
structure and classify it as packed retail data or host-owned runtime state.
For packed structures, preserve 32-bit slots under `XENO_PC_PORT` and use
explicit conversion helpers; use native pointers only where construction and
all consumers are genuinely host-owned. Once that boundary is proven, make
`sound.c` the canonical owner, remove the three duplicate override copies, take
the TU off hold, and use generated oracle stubs for the remaining ASM surface.

The recommended representation follows the existing packed-u32 rule, but the
per-structure packed-vs-host-owned classification is a required evidence pass,
not something this survey can infer safely.

## Decisions and remaining human calls

The three subsystems do not need one universal mechanism:

- **Menus:** ordinary decomp compilation plus typed oracle stubs.
- **Work-list:** permanent whole-TU PC replacement with one packed-u32 model.
- **Sound:** shared decomp TU with an audited `XENO_PC_PORT` representation
  boundary, then incremental stubs; retire historical duplicate definitions.

No human decision is required for menu ownership or the work-list runtime
layout. Two policy choices remain before sound implementation:

1. Confirm the project preference for packed-u32 access in raw sound objects
   rather than eagerly unpacking every asset into separate host-native objects.
   Packed access is recommended because it preserves retail offsets and matches
   the established work-list/field convention.
2. Decide whether the three duplicate sound functions should simply move back
   to `sound.c` ownership (recommended) or be guarded as explicit port variants
   if later evidence proves their host behavior must differ.

## Build-integrity finding from this survey

Game-TU compilation is fail-closed, but the port-only loop is not. A failed
`game_overrides.c`, `archive_port.c`, `work_list_port.c`, data source, or other
port source prints `FAILED` and continues without appending its object. The
trial link can then replace the missing real symbols with generated stubs.
`port_main.c` also reports failure without aborting or first deleting its old
object, so a stale object can survive. This is the same silent-substitution
failure class previously fixed for game TUs and should be repaired before
activating more subsystem owners.
