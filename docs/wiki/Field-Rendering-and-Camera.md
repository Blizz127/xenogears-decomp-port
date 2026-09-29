# Field Rendering and Camera

Camera, framing, and visibility findings from committed handoff notes. Only **proven** items are stated as fact; unresolved issues are labeled.

## Verified working (Map 0)

- Field overlay loads; main loop runs.
- Actor sprite primitives link into the OT via `func_8001E3D8` (`code=0x2d` `POLY_FT4`).
- `DrawOTag` runs once per frame; PsyCross `ParsePrimitive` reaches actor packets.
- User-confirmed visual recovery: sprite visible, fade, slow zoom (`work_list_port.c` fix).
- `D_800ADC18` fade-in gate: starts at 4, decrements per frame, clears at frame 4; gates `FieldAddPrimitives` but **not** direct actor OT linking.

## Committed layout / projection fixes

| Fix | What | Evidence |
|-----|------|----------|
| `FieldScene.unk7C` type | `u_long` → `u_int` on host (8-byte `u_long` broke offsets) | `&g_Scene.worldToScreenMatrix == g_Scene+0xD4` in gdb |
| `func_8007254C` defaults | `sceneScrZ=0x200`, `sceneDIP=0x1e`, `sceneScale=0x1000` | GTE `C2_H=512`; on-screen quad coords plausible |
| Z-clamp in `func_8007CD80` | Inverted bounds clamp corrected | Map1 actor visible milestone (Jul 5) |
| `func_800764B4` active-actor quad | Full billboard path implemented | Map1 executes real quad path (128 OT splices) |
| Model prim `0x0C` walker | `ModelPrimQuadF4Variant0` (0x18 stride) | Fixed corrupt prim codes from wrong GT4 walker |

## Map 1 rendering pipeline (decoded)

1. Field background from streamed archive `(mapNum<<1)+0xBB`.
2. **`PcPortDrain0xBBToVram`** (opt-in `XENO_FIELD_0BB_VRAM_UPLOAD=1`) uploads VRAM strips synchronously.
3. Model actors use world matrix with retail **`mvmva cv=0`** (translation included) — TR-add proven correct for `func_800748E8`.
4. Sprite actors use separate base matrix in `func_8001E3D8` (`pBase+0x0C`), not the actor-center matrix from `func_80075B44`.

## Camera investigations (proven)

### Camera target corruption (resolved)

- **Symptom:** Map1 black screen despite valid player spawn.
- **Root cause:** `func_80072A38` fallback corrupted camera target; compounded by inverted Z-clamp in `func_8007CD80`.
- **Status:** Z-clamp fix applied; Map1 actor visibility milestone reached.

### TR-add experiment (reverted, lesson retained)

- Adding `worldToScreenMatrix.t` to `actorMatrix.t` in `func_80075B44` was **wrong** for sprite path.
- Sprite quads project through `func_8001E3D8`'s own matrix (`pBase+0x0C`), which had independently bad translation.
- **Conclusion:** camera/sprite-base-matrix tracing was a symptom thread; camera-target/Z-clamp was the actual root cause for Map1 black screen.

### Model actor TR-add (proven retail-correct)

- `func_800748E8` asm uses GTE `mvmva cv=0` — `modelMatrix.t = R × pos + work.t`.
- C `ApplyMatrixLV` is rotation-only; TR-add reconstructs retail behavior.

## Unresolved / open issues

| Issue | Status | Notes |
|-------|--------|-------|
| Screen coordinate OT anomaly | **Unresolved** | OT1 frames: high 16 bits `0x7ffd`; OT2 frame: high byte `0x25`. Lower 32 bits consistent. Possible OT-switching or address-formation bug. |
| Default Map0 black | **Known separate** | Without `XENO_FIELD_MAP`; pre-existing at baseline `79c7ee7` |
| Permanent `0xBB` loader | **Not done** | Opt-in env flag; needs retail-shaped drain from `func_80070488` |
| Camera framing/scale cleanup | **Open** | Center-black region, tiny sprites noted in handoff |
| `func_80075B44` rare branches | **Assert-only** | Four real asm branches; not hit on Kernel0 route in bounded probes |
| Child sprites invisible | **Open** | Type-2 render callback `func_80025718` (`D_8004FD40[2]`) not ported; one-shot `[port]` log marks it; queued behind the anim-opcode frontier (see [Active Frontier](Active-Frontier)) |
| Map15 frame-117 full render | **Not yet proven** | Current abort happens mid-frame in the anim tick (`0xBC` sub `0x16`) |

## Temporary rendering hacks

| Hack | Location | Notes |
|------|----------|-------|
| `XENO_FIELD_0BB_VRAM_UPLOAD=1` | `archive_port.c` | Uploads streamed VRAM; default-off |
| PsyCross GPU patches | `build_port.sh` | Font `0xFC` mask, `dfe` always on-screen, DR_MODE full length |
| `USE_EXTENDED_PRIM_POINTERS=0` | `build_port.sh` | Simple primitive pipeline for this title |

## Systems trusted for implementation guidance

- `D_800ADC18` lifecycle (init → per-frame decrement → render gating).
- `FieldScene` layout on host must mirror 4-byte PSX types.
- GTE `cv=0` vs `cv=3` distinction in handwritten asm — audit item for future ports.
- Actor sprite draw is **not** blocked by `D_800ADC18`; only `FieldAddPrimitives` is.
