# Field Control and Movement

Input delivery, player-control opcode, walkmesh, collision, and verified locomotion milestones.

## Verified milestone — visible field control

**Fei is visibly controllable with real keyboard input on a rendered Map1 field.**

- **Commits:** `55a2dda`, `6bc1752`, `b9266c5` (documented)
- **Chain (all verified end-to-end):**

  | Step | Function / component | Status |
  |------|---------------------|--------|
  | Input delivery | SDL → `D_800AFE9C` d-pad state | ✅ |
  | Player-control opcode | `OP_UPDATE_CHARACTER` (0xA7) `func_8009F5F4` | ✅ `e7b0101` |
  | Animation / facing | `func_8001F8E8` non-delegating path | ✅ |
  | Walk-step vector | `func_80081F80` | ✅ `79c7ee7` |
  | Walkmesh triangle lookup | `func_8007B1C4` | ✅ `55a2dda` |
  | Walkmesh movement | `func_8007BAC0` | ✅ `55a2dda` |
  | Collision auto-move | `func_80082620` + `func_800825AC` | ✅ `6bc1752` |
  | On-screen result | Position advances; Fei visible | ✅ |

### Demo command

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  SDL_VIDEODRIVER=x11 XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 \
  XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=0 XENO_FIELD_0BB_VRAM_UPLOAD=1 \
  ./pc_port/build_native/xeno-port'
```

- Fei = **actor 1** (`g_PlayerActorIndex=1`)
- Arrows = D-pad; **C** = run/Cross
- `XENO_FIELD_ENTRANCE=8` also works
- Needs live X display for real keyboard input

## OP_UPDATE_CHARACTER (opcode 0xA7)

- Handler: `func_8009F5F4` in `src/field/main/misc6.c`
- Retail gate: `scriptFlags` (offset 0x00) bit `0x4000` = player-controlled
- **Bug found:** port originally checked `flags` (offset 0x04) instead of `scriptFlags`; player branch was missing
- **Fix committed:** `e7b0101` — player branch from retail asm; direction from d-pad nibble → LUT → camera angle → facing

### Direction input map (documented)

Synthetic injection via `D_800AFE9C`:

| Value | Direction |
|-------|-----------|
| `0x1000` | +X |
| `0x2000` | +Z |
| `0x4000` | −X |
| `0x8000` | −Z |

## Walkmesh and collision (committed)

| Function | Role | Commit |
|----------|------|--------|
| `func_80081F80` | Walk-step vector from facing/input | `79c7ee7` |
| `func_8007B1C4` | Walkmesh triangle lookup | `55a2dda` |
| `func_8007BAC0` | Walkmesh movement resolution | `55a2dda` |
| `func_80082620` | Collision-aware auto-move (slope, follow, work-object paths) | `6bc1752` |
| `func_800825AC` | 2D distance between field actors | `6bc1752` |

## Spawn / entrance harness

- `XENO_FIELD_ENTRANCE=N` writes `D_8006F954` → field script var 2 → spawn table index
- Valid documented entrances: **0, 8, 9** (among others in spawn table)
- **Invalid:** entrances **6, 10** — out-of-range walkmeshId; crash

## Encounter side-path (not locomotion blocker)

- `func_80079288` — random encounter management (177 insns)
- Called from `OP_UPDATE_CHARACTER` during held input after exit op54
- Generated stub returns normally; **not** the missing map-transition step
- Do not port full battle path for transition hunt (handoff classification)

## Current limitations

| Limitation | Notes |
|------------|-------|
| Map0 default route | Black screen without `XENO_FIELD_MAP` — separate issue |
| Synthetic vs real input | Exit-route proofs use gdb injection; milestone uses real keyboard |
| `func_80079288` stub noise | Encounter bookkeeping fires per held-input frame |
| Battle movement | `XENO_KERNEL_SEL=1` hits `func_8001B6C4` stub immediately |
| Walkmesh navigation to exit boxes | Some zones reachable; the Map1->Map15 transition chain is verified via the zone-5 reload probe (July 9); the natural door path is still gated by the rand()/var0x0408 issue (see [Current Status](Current-Status)) |

## Map1 movement proofs (read-only)

- Zone 11 reachable from entrance 9 with documented d-pad route
- Zone 8: X/Z reachable but height gate fails at `y=-1`, `y0=-106`
- Zone 9: height-valid; reachable with longer route (frame 533 proof)
- Wall-stuck paths documented for some entrance/heading combinations
