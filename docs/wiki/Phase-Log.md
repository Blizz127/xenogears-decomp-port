# Phase Log

Chronological summary of **major committed milestones** and documented investigation checkpoints. Uncommitted read-only proofs are marked.

For session-level detail see [`ACTIVE_HANDOFF.md`](https://github.com/Blizz127/xenogears-decomp-ai/blob/main/docs/ai_context/ACTIVE_HANDOFF.md).

---

## 2026-07-05 — Field visual recovery (Map 0)

| When | Milestone | Commit / evidence |
|------|-----------|-------------------|
| Jul 5 | Work-list path restored; sprite/fade/zoom visible again | `work_list_port.c`; user visual confirmation |
| Jul 5 | `FieldScene` host layout fix (`unk7C` size) | Handoff "Last Changes" |
| Jul 5 | `func_8007254C` projection defaults (`sceneScrZ`, `sceneDIP`, `sceneScale`) | Handoff runtime evidence |
| Jul 5 | `func_8009AD6C` actor direction handler; actor 18 correctly hides | `2627286` |
| Jul 5 | `XENO_FIELD_MAP` harness selector added | `port_main.c` |
| Jul 5 | Map1 stack-smash in `func_80080A74` identified | Read-only; later fixed |
| Jul 5 | `func_800764B4` active-actor quad path implemented | Committed in session |
| Jul 5 | Anim-script opcode 0xA0 implemented (bounded) | Handoff Jul 5 entry |
| Jul 5 | `XENO_FIELD_ENTRANCE` spawn selector (`D_8006F954`) | Handoff Jul 5 entry |
| Jul 5 | Map1 Z-clamp bug in `func_8007CD80` fixed; actor visible | Handoff Jul 5 VISUAL MILESTONE |
| Jul 5 | Map1 camera target corruption root-caused (`func_80072A38`) | Read-only → clamp fix |

## 2026-07-06 — Map1 rendering / VRAM pipeline

| When | Milestone | Commit / evidence |
|------|-----------|-------------------|
| Jul 6 | Field background uploader `func_8002BF38` ported | Handoff VRAM section |
| Jul 6 | Model prim `0x0C` F4 walker fix (`ModelPrimQuadF4Variant0`) | Handoff Bug 6 |
| Jul 6 | `PcPortDrain0xBBToVram` opt-in upload (`XENO_FIELD_0BB_VRAM_UPLOAD=1`) | `archive_port.c` |
| Jul 6 | TR-add proven retail-correct for model matrix (`func_800748E8`) | Read-only proof |

## 2026-07-07 — Player control and field progression

| When | Milestone | Commit / evidence |
|------|-----------|-------------------|
| Jul 7 | `func_8001F8E8` non-delegating sprite-frame path | Committed before 0xA7 |
| Jul 7 | OP_UPDATE_CHARACTER (opcode 0xA7) player branch | `e7b0101` |
| Jul 7 | Walk-step vector `func_80081F80` | `79c7ee7` |
| Jul 7 | Walkmesh triangle lookup + movement | `55a2dda` |
| Jul 7 | Collision auto-move `func_80082620` + `func_800825AC` | `6bc1752` |
| Jul 7 | **First visible field control** — real keyboard moves Fei on Map1 | `b9266c5` |
| Jul 7 | Map1 zone 5 encounter trigger fires (read-only) | `94d8cfc` (docs) |
| Jul 7 | Map1 exit/transition system found (zones 8–11) | `0e2c181` (docs) |

## 2026-07-08 — Map1 exit transition investigation

| When | Milestone | Commit / evidence |
|------|-----------|-------------------|
| Jul 8 | Zone 11 physically reachable; height gate passes | Read-only gdb |
| Jul 8 | Opcode 7 stall: slot-init offset bug in `func_80080A74` | `213b826` (docs) |
| Jul 8 | Slot-init fix exposed `func_80098CAC` pSprite bug | `7441ca9`, `ef1015f` (docs) |
| Jul 8 | Slot init + pSprite fixes; op7 advances to op116/op54 | `9e1b667` |
| Jul 8 | `func_800855C8` classified as sound-side | `36f195d` (docs) |
| Jul 8 | Field transition sound shim committed | `ae8c753` |
| Jul 8 | `func_80079288` classified as encounter bookkeeping | Docs on `ae8c753` |
| Jul 8 | Var `0x0462` decoded as local exit latch | `4856738`, `e13c0e3` (docs) |
| Jul 8 | Actor 18 routine 4 traced — door animation only | `bb31b27` (docs) |
| Jul 8 | Actor 18 routine 1 gated by `var0x0408 == 1` | `0545ef3` (docs) |
| Jul 8 | Actor 20 owns `var0x0408`; host rand range mismatch | `8db4fb2` (docs) |

## 2026-07-09 — Map1→Map15 reload: anim-script VM cleared, frame 117 reached

| When | Milestone | Commit / evidence |
|------|-----------|-------------------|
| Jul 9 | `fe8933f` FieldFree fix verified working — reload `HeapAlloc(171056)` succeeds; frontier moved to anim opcode 0xC6 | `f48a4ad`; zone-5 injection + heap-free-list-walk probes (scratchpad, not committed) |
| Jul 9 | Anim opcodes 0xC6/0x96/0xFC/0xE0/0x8D + child-sprite spawn subsystem (~16 functions); `WorkListEntry` PSX 0x1C-byte layout fix (latent native-pointer bug) | `ce61f3d`, `b21f164` (docs) |
| Jul 9 | Child model-data load opcodes 0xF5/0xF6 + `func_8002C59C` relocator; first child's script completes | `11b2474`, `9f5aaa1` (docs) |
| Jul 9 | Opcode 0xA3 (gravity setter → sprite `+0x1C`) | `f24e035`, `261343f` (docs) |
| Jul 9 | Opcode 0xBC sub-command 0x24 (multi-command position opcode, bounded — other 38 subs assert) | `de23142`, `494b18f` (docs) |
| Jul 9 | **MILESTONE:** opcode 0x94 (inherit-parent-rotation) — map15 **load-time script VM completes**; first post-load frame 117 begins | `a389755`, `5a75ae2` (docs); `map1_opcode_94_reload_20260709` log |
| Jul 9 | `func_80083288` "POLYCHECK" interaction-region test + `func_80084158` flags4-0x80 branch migrated; frame-117 actor update completes for all actors | `3858bd8`, `832dabd` (docs); `map15_polycheck_20260709_run3.log` |
| Jul 9 | Opcode 0xBC sub-command 0x25 (clear sticky bit); frontier = 0xBC sub-command 0x16 (move-to-parent, decoded) | `ae30d53`, `3442f3f` (docs); `map15_bc25_20260709_run1.log` |

---

## 2026-07-13 — Map014 fidelity, PsyCross fixes, and build-path audit

| When | Milestone | Commit / evidence |
|------|-----------|-------------------|
| Jul 13 | Camera angle, shadow FT4, and retail E688 cull sequence corrections | `1753a6a`, `751a194`, `640f79a` |
| Jul 13 | E688 OT depth corrected to retail min-SZ; actor-38 buckets match `74/89/87/73` | `fea685a`; frame-60 bucket probe |
| Jul 13 | RotAverage4, RotTransPers4, and NormalClip exact-matched; four bypassed PsyQ wrappers matched for decomp completeness | direct objdiff `{}` |
| Jul 13 | PsyCross raw-texture dither and paletted ABR bit-15 fixes made durable patches | Map000 live repro and generated-build validation |
| Jul 13 | Map-specific launchers, symbol-keyed probes, and liveness conventions added | session infrastructure commits |
| Jul 13 | Build audit found silent failed-TU skipping and non-iterating stale-stub reuse | open in `OPEN_ISSUES.md` |
| Jul 13 | Passive animation-opcode survey transport added; Map000/001/014 startup saw no target opcode | sentinel-backed fd-3 records; coverage limited |

## 2026-07-14 — Matrix alias correction

| When | Milestone | Commit / evidence |
|------|-----------|-------------------|
| Jul 14 | Corrected the earlier incomplete `CompMatrix` alias audit: rotation was safe, but PsyCross overwrote `m0.t` before rereading it when `m0 == m2` | Retail `0x80049440-0x8004946C`; synthetic `[110,220,330]` → pre-fix `[20,40,60]` |
| Jul 14 | PsyCross `CompMatrix` now preserves retail read-before-store ordering through a durable patch | `af122d1`; four alias/control assertions; fresh-worktree regeneration check |
| Jul 14 | No current runtime exposure found across five maps | 3,493 calls, zero `m0==m2`; Map014/047/334 tripwires unchanged |
| Jul 14 | `func_800748E8`'s four omitted retail cases are fully restored: parent composition and status mode 1 live-validated; axis rotations asm-proven; optional matrix asm + synthetic | `7b15b57`, `b5033a4`, `95ea1c9`, `eb45b66`; Map47 1,239 parent calls, Map334 236 mode-1 calls |
| Jul 14 | Raw-overlay `func_801E72CC` replaced its generated stub and the complete optional-matrix chain now preserves `[110,220,330]` end to end | `95ea1c9`, `af122d1`, `eb45b66`; stub count 199→198 |

---

## 2026-07-20 → 2026-07-26 — Menu nav arc (N1 → N2c-4): Items becomes usable

Full detail, including the parked debts and the four red pins, is in the nav-arc entry in
[`OPEN_ISSUES.md`](../../OPEN_ISSUES.md).

| When | Milestone | Commit / evidence |
|------|-----------|-------------------|
| Jul 20 | **N1 cursor-move** — input reader `func_801C7D78` + cursor rebuild; cursor moves, live keyboard drivable. Fixed two latent bugs in shipped render code (`func_801C55A0` `$a1` residue; raw `g_Menu+0x6E0` vs native inflation) | `7dd8459` |
| Jul 21 | **N2a confirm/cancel** — dispatcher `func_801C531C` + Items lifecycle `func_801DBE54`. Circle opens Items (SFX `0x5B`), Cross closes (`0x5C`). Window destruction is owned by the cleanup dispatcher, not the screen | `c3a9275` |
| Jul 21 | **N2b-1 names + counts** — real item names and two-digit quantities in window 3, paged; `GetItemName` corrected to keep PSX-width `lw` pointer semantics | `d82c428` |
| Jul 21 | **N2b-2 description + cursors** — description text in window 4; animated cursors (36 px in a 14×19 region) | `4aa01c7` |
| Jul 22 | **N2c-1 reorder** — `func_801DBD4C`, **26/26 byte-match**. First save-backed mutation; proven by framebuffer *and* `g_GameState` readback | `48ea933` |
| Jul 23 | **N2c-2a prompt UI** — twelve functions; target prompt with default target, clean cancel. Fixed the `func_801DBE54` confirm callsite; renamed `effectFailed` → `anyEffectApplied` (polarity was inverted) | `e4bb5c5` |
| Jul 24 | **N2c-2b navigation** — `func_801D9704` (65i, 7 callers); wraparound + ineligible-skip. Byte-match attempted, **not** achieved → coexistence pool | `062f22c` |
| Jul 24 | **N2c-3 ordinary effects** — `func_801E31C0`, 246/255 matched. HP/MP restore, stat boosts, clamping. `hp 20/50 → 50/50`, qty `1→0`, ID cleared, chime `0x37` | `8cbb3c8` |
| Jul 24 | **Toolchain diagnostic + build gate** — `jlabel` visibility `local → global` (one word) unbroke every jump-table label; `labels.inc`/`gte_macros.inc` now tracked. `make rom-check` added | `cdc8dd1`, `57db1d9` |
| Jul 26 | **N2c-4 magnitude-1 bulk** — `func_801E5058`, **72/72 byte-match**. Five inventory families at qty 10; 940 writes / 0 stray over a full `0x22B8` `g_GameState` dump, expected image parsed from the retail `.s` | `b3bd4a2` |

**Net:** Items is functionally complete for every reachable path. Special item use is
complete for **magnitude 1 only** — magnitude 2 (`func_801E5178`) is parked and
structurally absent from the port binary.

**Not green:** four pinned overlays (`slus_006.64`, `field.bin`,
`member_change_menu.bin`, `shop_menu.bin`) are known-red source regressions with named
causes. `make rom-check` reports them honestly; they are not fixed.

## Milestone demo command (field control)

Documented in `b9266c5`:

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  SDL_VIDEODRIVER=x11 XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 \
  XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=0 XENO_FIELD_0BB_VRAM_UPLOAD=1 \
  ./pc_port/build_native/xeno-port'
```

Fei = actor 1. Arrows = D-pad; C = run/Cross. `XENO_FIELD_ENTRANCE=8` also works.

## Protected baselines

- **Map0 visual baseline:** 6 sprite actors `1, 2, 16, 23, 25, 26`; actor 18 hidden by script.
- **Do not use entrances 6 or 10** on Map1 — harness-invalid spawn indices.
