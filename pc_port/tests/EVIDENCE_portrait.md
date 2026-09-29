# Field dialogue portrait diagnosis

Date: 2026-09-03  
Checkout: `experiment/worldmap-open-gates-20260823`, HEAD `3a3e7aac` (`docs: log title backdrop, text engine, and Map 2 OT fixes`).

This is a diagnosis-only report. The checkout already contains unrelated dirty
work; no production source was changed for this report.

## Finding: first evidenced failure

The first evidenced failure is in the **render/OT stage**, not in face-ID
selection or the archive read path. `func_8007E1C0` currently computes the
window geometry, links the border primitives, and links the background tile and
draw mode, then returns at [`src/field/dialogue/text_box_render.c:166-279`](../../src/field/dialogue/text_box_render.c#L166). There is no portrait-width/height clamp, no call to `func_8007E16C`, no read of `portrait.shouldRenderPortrait`, and no portrait polygon/draw-mode OT insertion in this function.

Retail `field.bin` contains that missing work. Reproducible command:

```sh
mips-linux-gnu-objdump -D -b binary -m mips:3000 -EL \
  --adjust-vma=0x8006faf0 \
  --start-address=0x8007e1c0 --stop-address=0x8007ee0c \
  disc/field.bin
```

Relevant retail addresses from that disassembly:

| Retail address | Observed operation |
| --- | --- |
| `0x8007E8E0-0x8007E918` | Clamp portrait width/height to `0x40` (or window dimension minus `8`). |
| `0x8007E934-0x8007E968` | Compute portrait x/y; apply the `flags & 0x20` horizontal flip path. |
| `0x8007E96C-0x8007E9A8` | Select the per-render-context `POLY_FT4` and call `func_8007E16C`. |
| `0x8007E9AC-0x8007E9BC` | Load `shouldRenderPortrait` from absolute `0x800C2B2C` (`box + 0x494`) and require `== 1`. |
| `0x8007E9C8-0x8007EA48` | Insert the portrait polygon and its draw mode into the ordering table (two OT insertions). |

Therefore a textbox can have a valid face ID, UVs, CLUT, and loaded TIM data,
but the current C path still submits no portrait primitive. This is a direct,
source-backed explanation for missing portraits. It is earlier and more
conclusive than speculating about Linux/GL texture behavior.

## Upstream chain audit

The upstream data path is present in the current tree:

1. `func_8007F8DC` selects the face direction and calls `func_8007F5AC`, then
   sets `shouldRenderPortrait = 1` and `portraitID = faceId` when the talking
   actor has a face; the no-face path clears the flag at
   [`src/field/dialogue/text_box_render.c:524-535`](../../src/field/dialogue/text_box_render.c#L524).
2. `func_8007F5AC` writes the 64x64 UV rectangle and CLUT to both
   per-context portrait polygons at
   [`src/field/dialogue/text_box_render.c:400-426`](../../src/field/dialogue/text_box_render.c#L400).
3. `func_8009C154` maps `faceId` through `D_800AE1E0`, adds archive directory
   offset `0x46`, allocates one or two TIM buffers, fills `D_800B00C8`, and
   queues the read at [`src/field/dialogue/text_box.c:201-294`](../../src/field/dialogue/text_box.c#L201).
4. On a pending slot, it polls `ArchiveDataSync()` and then calls
   `FieldLoadTIMWithClut` with the slot's `D_800AEAE4` coordinates at
   [`src/field/dialogue/text_box.c:213-229`](../../src/field/dialogue/text_box.c#L213).
5. `D_800AE1E0` is the 90-face archive-pair table and `D_800AEAE4` is the
   four-slot VRAM/CLUT coordinate table at
   [`pc_port/src/data_field.c:104-126`](../../pc_port/src/data_field.c#L104).

The current PC compatibility layer does parse TIM headers and exposes the
`OpenTIM`/`ReadTIM` result used by `FieldLoadTIMWithClut` at
[`pc_port/src/psyq_compat.c:62-126`](../../pc_port/src/psyq_compat.c#L62). No
runtime log in the available opening/Lahan scratchpads records a portrait TIM
parse failure, a bad TIM ID, or an archive error. A live portrait-bearing
dialogue capture is still required before claiming the archive stage is proven
healthy; it is not needed to establish the render-stage omission above.

## LP64 queue/layout audit

`StreamDataQueueEntry` is declared with a native `void *` at
[`include/system/archive.h:45-49`](../../include/system/archive.h#L45). On the
PSX its layout is 8 bytes (`archiveIndex` at `+0`, padding at `+2`, 32-bit data
pointer at `+4`); on this LP64 build it is 16 bytes with `pData` at `+8`.
Measured with the current headers and `-DUSE_EXTENDED_PRIM_POINTERS=0`:

```text
sizeof(StreamDataQueueEntry)=16 archiveIndex=0 pad=2 pData=8
primitive sizes DR_MODE=12 SPRT=20 TILE=16 POLY_FT4=40
sizeof(FieldTextBoxPortrait)=108 ... should=104 id=105
sizeof(FieldTextBox)=1176 portrait=1068 (0x42c)
```

The text-box structure is therefore retail-sized (`0x498`) under the port's
primitive configuration, while the queue type is not. The port explicitly
handles this split: `func_80029AFC` uses a packed 8-byte reader only for queues
in emulated PSX RAM, and otherwise walks native `StreamDataQueueEntry` values at
[`pc_port/src/archive_port.c:275-337`](../../pc_port/src/archive_port.c#L275). The
portrait queue storage is deliberately oversized for three native entries at
[`pc_port/src/data_field.c:818-821`](../../pc_port/src/data_field.c#L818).
This is a portability risk that must remain covered by a queue-layout test, but
it is not the first evidenced portrait failure: the native queue branch does
read `pData` and invoke `ArchiveReadFileToBuffer`, whereas the renderer never
links a portrait primitive at all.

`ArchiveDataSync` itself returns busy only while the archive state machine is
non-idle at [`src/slus_006.64/system/libarchive.c:186-199`](../../src/slus_006.64/system/libarchive.c#L186), and the port clears the file size/error/state after each synchronous read at
[`pc_port/src/archive_port.c:393-398`](../../pc_port/src/archive_port.c#L393). Thus
there is no static evidence that readiness polling is the first failure.

## Focused regression test proposal

Add `pc_port/tests/portrait_render_prod_test.c` with a small shell wrapper in
the existing `pc_port/tests/run_*` style. The test should compile the current
`text_box_render.c` with `-DXENO_PC_PORT -DUSE_EXTENDED_PRIM_POINTERS=0` and
minimal PsyCross/field stubs, then:

1. Initialize one textbox's two portrait polygons/draw modes.
2. Set geometry, `flags` (including a flip case), and
   `portrait.shouldRenderPortrait = 1`; call `func_8007E1C0` for render contexts
   0 and 1.
3. Assert that the selected context's portrait polygon is positioned, that its
   dimensions are clamped to 64 (or window minus 8), and that the OT head changes
   through the portrait polygon and portrait draw mode.
4. Set `shouldRenderPortrait = 0` and assert that no portrait OT nodes are added.
5. Include negative controls that remove the `func_8007E16C` call and the
   `shouldRenderPortrait` gate; each mutant must fail the test.

Keep a separate queue-layout assertion (`sizeof(StreamDataQueueEntry) == 16`,
`offsetof(pData) == 8` on LP64) so a future refactor cannot silently route a
native queue through the packed PSX reader. This test should remain source/data
driven; no hardcoded portrait asset or substitute texture is acceptable.

## Verification status

The current dialogue translation units pass syntax-only compilation:

```text
gcc ... -fsyntax-only src/field/dialogue/text_box.c \
             src/field/dialogue/text_box_render.c
syntax-only: PASS
```

The proposed render test is **NOT IMPLEMENTED** in this diagnosis-only task.
Runtime proof of a portrait-bearing dialogue reaching `func_8009C154`, completing
`ArchiveDataSync`/`FieldLoadTIMWithClut`, and then producing the missing OT nodes
remains **PENDING**. The static first failure remains the absent portrait
position/gate/OT block in `func_8007E1C0`.
