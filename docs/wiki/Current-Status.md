# Current status

The numbers for this tree are on [Decompile status](Decompile-Status) and [Port status](Port-Status). Those pages match `main`. The notes under the 2026-08-01 heading are older field milestones. They are not a second set of totals. `OPEN_ISSUES.md` and `docs/ai_context/` are not on `main`.

As of the published tree:

- The matching build's function split has no bare unported row. Field (878) and the member-change menu (67) are plain C. Battle still has 506 `INCLUDE_ASM`s with no port body.
- The port links 58 game translation units. Psy-Q is PsyCross. 169 battle TUs are reference-only.
- Field play, including the Blackmoon Forest route, is the exercised native path. Battle beyond the host-leaf allowlist is the interpreter. The world map mode is not the retail world-map overlay.

This page answers six questions the project tracks constantly.

---

## 2026-08-01 update — F16 field-to-world boundary is complete; world map remains hollow

- `func_800798BC`, the complete four-case `func_8007954C` dispatcher, and its
  small exit-3 dependency `func_8001BB50` are real native symbols; their
  generated stubs are gone. Retail builds retain byte-identical assembly.
- The Lahan exit-1 route now completes the documented 12-call FieldMain
  teardown, selects state 3 directly, and arrives at a compiled native
  `WORLD MAP NOT YET PORTED` mode. State 3 uses the retail memory/heap offsets
  but `hasOverlay=0`, so archive `0x0F` and raw entry `0x80070CFC` are not
  executed on the host.
- The arrival consumes tuple `(0x0400,0x0E00,1,1)` and logs derived world index
  `0`, entrance `1`, retained arg2 `1`, and heading `0x0E00`. The hold-off
  regression remains stable for 120 Vsyncs; the hold-enabled F14 route still
  restores field readiness/control and stays live through frame 1020.
- This does **not** port the world map. It also does not fix the premature
  frame-814 trigger: `FieldProjectActorOriginToScreen` is still a generated
  stub and is the next bounded field task; retail triggers later near frame 916.

## 2026-07-14 update — build integrity is fail-closed; matching ELFs are blocked

- **Map014 room/sprite ordering:** `func_8002E688` now derives OT depth from
  retail `min(SZ0..SZ3) >> D_80050100`. At frame 60 actor 38 uses buckets
  `74/89/87/73`, behind Fei at `41`; the temporary FLAG guard was discarded.
- **PsyCross fidelity:** raw-textured dithering and the paletted ABR 1/2/3
  CLUT-bit-15 gate are durable build patches. The latter has a live Map000
  frame-33 repro; ABR-0 remains unchanged. `CompMatrix` also now preserves
  retail's translation read-before-store ordering when `m0 == m2`; the prior
  alias audit had checked rotation only and was incomplete (`af122d1`).
- **Build integrity:** unexpected game-TU compile errors now abort. The four
  historical skips are understood as three blocked subsystem workstreams:
  sound has 39 unresolved `INCLUDE_ASM` functions, the two menu overlays have
  15, and work-list needs a host-layout routing decision.
- **Matching ELFs:** the documented MIPS toolchain is available, but the
  matching build fails at the main SLUS link because `psyq/libgte.c` emits
  unresolved inline `gte_*` helpers. No matching ELF can be generated until
  that build-integration issue is repaired. See
  [`OPEN_ISSUES.md`](../../OPEN_ISSUES.md).
- **Animation-opcode survey:** a gated fd-3 trace saw no dedicated-unimplemented
  opcode in passive 10-second Map000/001/014 starts. That is startup coverage,
  not proof that the opcodes are dead.

## What currently works

### PC port infrastructure

| Area | Status | Notes |
|------|--------|-------|
| Native build/link | **Fail-closed; subsystem workstreams open** | Unexpected TU failures abort. Sound/menu activation is blocked by unresolved `INCLUDE_ASM` surface; work-list requires routing work |
| PsyCross integration | **Verified working** | SDL2/OpenAL/OpenGL; documented patches in `build_port.sh` |
| Boot-path stub oracle | **Verified working** | Undefined symbols auto-stubbed; live path logs `[stub] <name>` |
| Kernel0 field route | **Verified working** | `XENO_KERNEL_SEL=0` reaches `FieldMain`, times out cleanly (`RUN_RC=124`) |

### Map 0 (default field harness)

| Area | Status | Notes |
|------|--------|-------|
| Field overlay load | **Verified working** | Field main loop runs |
| Actor sprite draw path | **Verified working** | `func_8001E3D8` links `POLY_FT4` into OT; user-confirmed sprite/fade/zoom |
| Work-list path | **Committed fix** | `pc_port/src/work_list_port.c` restored real work-list functions |
| `D_800ADC18` fade gate | **Decoded / trusted** | 4→0 over frames 0–4; `FieldAddPrimitives` gated behind it |
| Map0 actor draw list | **Verified working** | 6 visible sprite actors: 1, 2, 16, 23, 25, 26 (actor 18 correctly hidden) |

### Map 1 (field test harness)

| Area | Status | Notes |
|------|--------|-------|
| Map1 load/render | **Verified working** | With `XENO_FIELD_MAP=1` + entrance selector; Fei visible on terrain |
| Field background VRAM | **Temporary hack (opt-in)** | `XENO_FIELD_0BB_VRAM_UPLOAD=1` uploads streamed `0xBB` archive |
| Visible player control | **Verified working** | Real keyboard input moves Fei on rendered field (`b9266c5` milestone) |
| Walkmesh + movement chain | **Committed fix** | Triangle lookup, walk step, collision auto-move (`55a2dda`, `6bc1752`, `79c7ee7`, `e7b0101`) |
| Encounter trigger zones (2D) | **Proven (read-only)** | Zone 5 fires `func_80093B10` enable-encounters path (`94d8cfc`) |
| Exit trigger zone 11 reachability | **Proven (read-only)** | Height-valid zone 11 fires opcode 203 inside path |
| Exit op7 → op116 → op54 | **Committed fix** | Slot init + sprite pointer + sound shim (`9e1b667`, `ae8c753`) |

### Map1 → Map15 seamless reload (July 8–9, 2026 campaign)

Driven by the zone-5 input-injection probe with `XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=8`: the probe gdb-injects synthetic d-pad input (`D_800AFE9C = 0x2000`, +Z) at `misc2.c:1688`, gated to pre-reload frames; Fei walks into Map1's zone-5 trigger, whose script chain (the A50 script) issues CHANGE_FIELD (op 152) through the retail path — the transition itself is not injected. From the `0xE0` pass onward the probe file is `captures/render_diag/map1_opcode_e0_reload_20260709.gdb` (earlier passes used scratchpad-only probes). The full chain — FieldLoad #1 (map1) → CHANGE_FIELD → FieldFree → FieldLoad #2 (map15, frame 116) → load-time script VM → frame 117 — now runs end-to-end.

| Area | Status | Notes |
|------|--------|-------|
| CHANGE_FIELD transition chain | **Verified working** | Music-ready shim + `func_800932D0` (VM op 152) + `func_800854D0` archive gate; CHANGE_FIELD completes end-to-end (`60035b6`, `fe66d9e`, `7acb74a`, `3c8f954`) |
| `func_800A5C40` + `FieldFree` | **Committed fix** | Transition orchestrator (all 7 branches) + `FieldFree` implemented (`4b377be`); FieldFree per-actor cleanup bug fixed — wrong-width pointer reads + two skipped frees (`fe8933f`) |
| Heap reclaim across reload | **Verified working** | Reload `HeapAlloc(171056)` succeeds, no OOM (`f48a4ad`) |
| Load-time anim-script VM | **Verified working** | Full opcode chain `0xC6 → 0x96 → 0xFC → 0xE0 → 0x8D → 0xF5/0xF6 → 0xA3 → 0xBC/0x24 → 0x94` cleared; 3 type-2 child sprites spawn, tick, and complete their scripts (`ce61f3d`, `11b2474`, `f24e035`, `de23142`, `a389755`) |
| First post-load frame 117 | **Verified working** | `FieldLoad` returns; map15's first post-load frame begins (`a389755`; `map1_opcode_94_reload_20260709_run1.log`) |
| POLYCHECK actor interaction | **Committed fix** | `func_80083288` mesh containment test + `func_80084158` flags4-0x80 branch live at frame 117, incl. real miss-path test vs actor 32's mesh (`3858bd8`; `map15_polycheck_20260709_run3.log`) |
| Per-frame child pump | **Verified working** | `func_800752C8` → `TimerWorkListUpdate` ticks children each frame (ticks=8 observed) |
| WorkListEntry PSX layout | **Committed fix** | Pointer fields now u32 (PSX 0x1C-byte layout), fixing a latent garbage-callback bug; same fix in `func_8001CE74` (`ce61f3d`) — see [Matching and Porting Rules](Matching-and-Porting-Rules) |
| Child-sprite rendering | **Queued** | Children tick but do NOT render — `func_80025718` (type-2 render callback, `D_8004FD40[2]`) not ported |

---

## What was proven

- **End-to-end visible field control** — input → opcode 0xA7 → animation → walkmesh → collision → on-screen movement (`b9266c5`).
- **Map1 has two trigger systems** — encounter zones 0–7 (2D) vs exit zones 8–11 (3D height-gated) (`0e2c181`, `94d8cfc`).
- **Opcode 7 stall root cause** — `func_80080A74` slot-init offset bug made idle actor script slots appear busy (`213b826`); fixed in `9e1b667`.
- **`func_80098CAC` pSprite bug** — masked slot-init issue; fixed in `9e1b667`.
- **`func_800855C8` is audio-side** — not a hard transition blocker; shim committed in `ae8c753`.
- **`func_80079288` is encounter bookkeeping** — exposed after op54 during held input; not the missing fade/map-load step (`ae8c753` docs).
- **Var `0x0462` is a local exit latch** on actor 48 routine 1, not a separate consumer (`4856738`, `e13c0e3`).
- **Actor 18 routine 4 is a short door animation** — not the map transition itself (`bb31b27`).
- **Shared door state var `0x0408`** — owned by actor 20 routine 1; actor 18 routine 1 branches on it (`0545ef3`, `8db4fb2`).
- **`FieldLoad` does NOT hang on reload** — the apparent hang was `HeapAlloc` exhaustion caused by a genuine `FieldFree` leak (wrong-width pointer reads + two skipped frees), fixed in `fe8933f` (`4f8ca5a` correction docs).
- **Map15 load-time script VM completes** — the anim-opcode frontier chain is done for this path; three type-2 children spawn, tick, and finish their scripts (`a389755` milestone, `5a75ae2`).
- **Child sprites tick but do not render yet** — only `func_80025710` of the render-callback table `D_8004FD40[16]` is decompiled; type-2 children need `func_80025718` (log-once "not ported" stubs cover the gap).
- **`func_8007CD3C` returns PSX scratchpad pointers** (`0x1F800000`-based) — unmapped on the port; convention is to keep push/pop calls balanced and back the workspace with a C local (found via live SIGSEGV; `832dabd`).
- **Retail `$sp`-switch trampolines cannot be expressed in C** — the `0xFC` upload chain elides the switch but keeps both `HeapAlloc(0x2000)`/`HeapFree` pairs so heap state stays retail-exact (`b21f164`).
- **Most July 9 passes were adversarially verified** against retail asm by independent refutation agents before commit (0xFC: 3 agents; 0xE0 chain: 5; 0xF5/0xF6: 2; 0xBC/0x24: 1; POLYCHECK: 1 — none refuted). Documented catches: the 0x8FE7→0xC3E7 diagnostic mask typo and a degenerate-loop convention fix; the scratchpad-pointer deref was caught by a live SIGSEGV during the POLYCHECK build, not by review. The 0xA3, 0x94, and 0xBC/0x25 passes were verified by live probe + smokes only.

---

## What is still broken

| Blocker | Severity | Notes |
|---------|----------|-------|
| **Anim opcode `0xBC` sub-command `0x16`** | **Active frontier** | Move-to-parent (snap child to parent position via `+0x70` back-link); Decoded (jtbl entry `0x8002044C`), trivial next pass; hit on the frame-117 child tick (`map15_bc25_20260709_run1.log`, `3442f3f`) |
| **Child sprites invisible** | Queued | `func_80025718` (type-2 render callback, `D_8004FD40[2]`) not ported — children tick but do not render |
| **Frame-117 full render not proven** | Unresolved | Abort happens mid-frame in the child tick pump; no complete frame/present of map15 observed yet |
| **Remaining `0xBC` sub-commands + paths** | Assert-only | Other 36 subs, the bit7-clear anchor path, and the camera-relative `ApplyMatrixSV` tail assert loudly by design; more subs may surface as the positioning preamble unwinds |
| **misc8 select-target / standing-on-top** | Assert-only | `.L80084520`/`.L80084570` machinery of the reduced `func_80084158` migration still asserts (3 deliberate asserts, `3858bd8`) |
| **Host `rand()` range mismatch** | Known separate | Actor 20's `var0x0408` chooser expects PSX `0..32767`; host glibc `rand()` writes out-of-range values → always falls back to `1` (`8db4fb2`); affects the natural door path — the reload campaign drives CHANGE_FIELD via the zone-5 probe |
| **Default boot Map0 black screen** | Known separate | Without `XENO_FIELD_MAP`, default route renders black — pre-existing, not a regression |
| **Battle route (`XENO_KERNEL_SEL=1`)** | Stubbed | `func_8001B6C4` is INCLUDE_ASM; immediate stub abort |
| **Direct menu route (`XENO_KERNEL_SEL=4`)** | Harness gap | `MenuMain` without overlay load hits `func_801C62A8` stub |
| **`func_80075B44` rare branches** | Assert-only | Four real asm branches not hit on Kernel0 route; not current gate |
| **Screen coordinate OT anomaly** | Unresolved | OT1 vs OT2 high-address bits differ; lower 32 bits consistent |
| **Entrances 6/10** | Harness-invalid | Out-of-range spawn-table walkmeshId; crash — do not use for milestones |
| **697 generated function stubs** | Ongoing | Boot-path oracle still reports missing functions on broader routes; new *soft* (log-once, non-blocking) stubs reached July 9: load path `func_8001B5E8`, `SoundFreeWdsEntry`, `func_8008E718`; frame 117 `func_80097954`, `func_8003A450`, `func_8008FB98` |

---

## Which fixes are committed

Recent committed milestones (see [Phase Log](Phase-Log) for full list):

### July 8–9 Map1 → Map15 reload campaign

| Commit | What |
|--------|------|
| `60035b6` | PC-port sound-readiness shim for the transition music-ready gate |
| `fe66d9e` | `func_800932D0` — CHANGE_FIELD field-VM opcode 152 |
| `7acb74a` | `func_800854D0` archive-completion gate; CHANGE_FIELD completes end-to-end |
| `4b377be` | `func_800A5C40` (FieldMain transition orchestrator, all 7 branches) + `FieldFree` |
| `fe8933f` | FieldFree per-actor model-data cleanup fix (wrong-width reads + two skipped frees) |
| `ce61f3d` | Anim opcodes `0xC6`/`0x96`/`0xFC`/`0xE0`/`0x8D` + child-sprite spawn subsystem + WorkListEntry PSX-layout fix |
| `11b2474` | Child opcodes `0xF5`/`0xF6` (script-embedded model-data load) + `func_8002C59C` relocator |
| `f24e035` | Child opcode `0xA3` (gravity setter) |
| `de23142` | Opcode `0xBC` sub-command `0x24` (multi-command position opcode, bounded) |
| `a389755` | Opcode `0x94` (inherit-parent-rotation) — **map15 load-time script VM completes; frame 117 begins** |
| `3858bd8` | `func_80083288` POLYCHECK interaction-region test + `func_80084158` flags4-0x80 branch |
| `ae30d53` | Opcode `0xBC` sub-command `0x25` (clear sticky bit) |

Docs/verification checkpoints for the campaign: `afb5535`, `12a01b9`, `3c8f954`, `5b3249c`, `4f8ca5a`, `f48a4ad`, `b21f164`, `9f5aaa1`, `261343f`, `494b18f`, `5a75ae2`, `832dabd`, `3442f3f`. Each code pass was built (`LINK OK`), run through the zone-5 reload probe, smoke-tested (ent8/ent0/Map0, `RC=124`, zero new stubs), and most passes were adversarially verified against retail asm before commit. Key logs: `captures/render_diag/map1_opcode_{fc,e0,8d,f5,a3,bc,94}_reload_20260709*.log`, `map15_polycheck_20260709_run3.log`, `map15_bc25_20260709_run1.log`.

### Earlier milestones (July 5–8)

| Commit | What |
|--------|------|
| `e7b0101` | OP_UPDATE_CHARACTER player-control opcode (0xA7) |
| `79c7ee7` | Walk-step vector `func_80081F80` |
| `55a2dda` | Walkmesh triangle lookup and movement resolution |
| `6bc1752` | Collision-aware auto-move `func_80082620` + `func_800825AC` |
| `b9266c5` | First visible field-control milestone documented |
| `9e1b667` | Actor script slot init + `func_80098CAC` sprite pointer |
| `ae8c753` | PC-port sound shim for field transition cue (`func_800855C8`) |

Docs-only investigation checkpoints (no code): `94d8cfc`, `0e2c181`, `213b826`, `7441ca9`, `ef1015f`, `36f195d`, `4856738`, `e13c0e3`, `bb31b27`, `0545ef3`, `8db4fb2`.

---

## Which hacks are temporary

| Hack | Env / location | Purpose | Retail-final? |
|------|----------------|---------|---------------|
| `XENO_KERNEL_SEL` | `psyq_compat.c` | Force kernel menu state (field/battle/menu) | No — test harness |
| `XENO_FIELD_TEST` | `port_main.c` | Skip boot, enter field test path | No |
| `XENO_FIELD_MAP` | `port_main.c` | Set `D_8006F94E` map selector | No — coverage harness |
| `XENO_FIELD_ENTRANCE` | `port_main.c` | Set `D_8006F954` spawn entrance index | No — spawn harness |
| `XENO_FIELD_0BB_VRAM_UPLOAD` | `archive_port.c` | Synchronous `0xBB` VRAM drain | No — opt-in; needs retail-shaped loader |
| `func_800855C8` PC-port no-op | `misc8.c` (`ae8c753`) | Skip deep sound chain on transition cue | Shim — documented audio boundary |
| Music-ready gate shim | `misc8.c` (`60035b6`) | Sound-readiness gate for CHANGE_FIELD without ported audio | Shim — documented audio boundary |
| Zone-5 reload injection probe | `captures/render_diag/map1_opcode_e0_reload_20260709.gdb` | Gdb-injects synthetic d-pad input (`D_800AFE9C = 0x2000`, +Z; pre-reload frames only) so Fei walks into zone 5 and the A50 script issues CHANGE_FIELD via the retail path — the transition itself is not injected; drives Map1→Map15 reload (0xE0 pass onward; earlier passes used scratchpad-only probes) | No — test driver, no source edits |
| Render-callback log-once stubs | `pc_port/src/game_overrides.c` | Missing `D_8004FD40[N]` slots log `[port] anim render callback ... not ported` once | Temporary until callbacks are decompiled |
| PsyCross GPU patches | `build_port.sh` | Font rendering, DR_MODE length, dfe draw | Port compatibility fixes |
| Generated function stubs | `pc_port/build_native/stubs.c` | Oracle for missing decomp | Temporary by design |

---

## Which systems are decoded enough to trust

| System | Trust level | Evidence |
|--------|-------------|----------|
| Field script VM dispatch | **High** | Two handler tables; byte `254` = extended opcode via `g_FieldScriptVMHandlers2` |
| Opcode 0xA7 `OP_UPDATE_CHARACTER` | **High** | Retail asm + committed implementation; `scriptFlags & 0x4000` gate |
| Opcode 7 actor-script start | **High** | `func_8009EB78` free-slot scan matches retail |
| Opcode 203 3D trigger zones | **High** | Height gate + inside path at `misc11.c:992` traced live |
| Actor script slot layout | **High** | Slot base `p+0x8C+i*8`; flag word semantics proven |
| Walkmesh movement chain | **High** | End-to-end position advance with real input |
| `D_800ADC18` fade-in gate | **High** | Init/decrement/gating traced in source |
| Map1 encounter zones 0–7 | **High** | All route to `func_80093B10` |
| Map1 exit zones 8–11 | **High** | Separate 3D height-gated system; zone 11 reachable |
| Actor 20 `var0x0408` state machine | **Medium-high** | Static decode + runtime writes traced; RNG cause identified |
| Camera Z-clamp (`func_8007CD80`) | **High** | Inverted clamp bug found and fixed (July 5 handoff) |
| Anim-script opcode dispatch (`func_8001FBE4`) | **High** | 10 opcodes (`0xC6`/`0x96`/`0xFC`/`0xE0`/`0x8D`/`0xF5`/`0xF6`/`0xA3`/`0xBC`/`0x94`) implemented asm-faithful in `animation_scripts.c` across 9 passes; most passes adversarially verified vs retail asm (July 9 chain) |
| Child-sprite spawn subsystem (opcode `0xE0`) | **High** | ~16 functions; live spawn + tick + script completion for 3 children on the map15 reload (`ce61f3d`) |
| Opcode `0xBC` sub-dispatch (`jtbl_800185A8`, 0x27 entries) | **Medium-high** | Subs `0x24`/`0x25` implemented, `0x16` decoded; remaining subs assert loudly |
| POLYCHECK mesh test (`func_80083288`) | **High** | Live miss-path test vs actor 32's mesh at frame 117 (`3858bd8`) |
| FieldFree heap reclaim | **High** | Reload `HeapAlloc(171056)` verified post-fix (`fe8933f`, `f48a4ad`) |
| Noah reference | **Low (research only)** | Non-matching; confirm against SLUS asm/runtime always |
