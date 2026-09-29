# Known Addresses and Globals

Human-readable table of important addresses and globals **already documented** in committed handoff notes. Status of the table below reflects handoff as of `8db4fb2`; the [July 9, 2026 additions](#july-9-2026-additions--animation-and-child-sprite-subsystem) reflect handoff as of `3442f3f`.

| Symbol | Meaning | Status | Evidence source |
|--------|---------|--------|-----------------|
| `D_8006F94E` | Field map selector; copied to `g_GameSceneMapNum` | Harness + decoded | Handoff `XENO_FIELD_MAP`; `port_main.c` |
| `D_8006F954` | Spawn entrance index → field script var 2 | Harness + decoded | Handoff `XENO_FIELD_ENTRANCE`; `port_main.c` |
| `D_800AFE9C` | Synthetic/real d-pad direction injection word | Verified | Handoff direction map; movement proofs |
| `D_800ADC18` | Field fade-in counter; gates `FieldAddPrimitives` | Decoded / trusted | `misc3.c:299` init; `misc4.c:21` decrement; `misc2.c:1508` gate |
| `D_800ADBEC` | Field transition / reload state | Observed | Exit transition handoff; `-1` at idle |
| `D_800B00C0` | VM stall flag | Observed | Trigger zone 5 proof |
| `D_800ADBDC` / `D_800ADBE4` | Encounter gate flags | Observed | Encounter trigger proof |
| `g_PlayerActorIndex` | Current player actor index | Verified | Fei = 1 on Map1 milestone |
| `g_FieldActors[]` | Field actor table | Decoded | Slot-init / op7 diagnostics |
| `g_FieldScriptVMHandlers` | Primary VM opcode table (0–255) | Decoded | `virtual_machine.c`; `data_field.c` |
| `g_FieldScriptVMHandlers2` | Extended VM table (256–482) | Decoded | Handoff VM encoding note |
| `g_Scene` / `FieldScene` | Scene state incl. `worldToScreenMatrix` | Fixed layout | `main.h` `unk7C` fix; gdb offset proof |
| `g_Scene.sceneScrZ` | Scene projection Z (`+0x68`) | Committed default | `func_8007254C` fix → `0x200` |
| `g_Scene.unk48` | Scene flags; encounter enable sets `0xC000` | Observed | Zone 5 trigger proof |
| `g_CamInterpolation` | Camera interpolation state | Decoded | Camera corruption investigation |
| `D_8004FBB8` | Matrix copy destination for projection | Observed | Pre/post `FieldScene` layout gdb |
| `D_800AFB54` | Actor count during field load | Suspected | Map1 stack-smash context (`func_80080A74`) |
| `D_800AFD1C` | Current script-move actor index | Observed | `func_80098CAC` fix context |
| `D_800B2290`–`D_800B229C` | Encounter cooldown / selection state | Classified | `func_80079288` diagnostic |
| `D_800595D8` | Sound channel records | Classified | `func_800855C8` / `func_8003A20C` asm |
| `D_80059404` | Sound control state | Classified | `func_80039F9C` asm |
| `g_GameSceneMapNum` | Current field map number | Observed | Map-load opcode family |
| `g_FieldControl` | Field control gate for player opcode | Observed | `OP_UPDATE_CHARACTER` runtime |
| `g_FieldScriptMemory` | Field script variable storage | Decoded | Var `0x0462`, `0x0408`, `0x0418` traces |
| Script var `0x0462` | Local exit latch (actor 48 routine 1) | Decoded | `4856738`, `e13c0e3` |
| Script var `0x0408` | Shared door state (actor 20 writes, actor 18 reads) | Decoded / trusted (July 8 frontier — superseded; see [Active Frontier](Active-Frontier)) | `0545ef3`, `8db4fb2` |
| Script var `0x0418` | Actor 18 local rotation counter | Decoded | Routine 1 state-2 branch |
| Script vars `1116`–`1122` | Exit zone "used" flags | Observed | Exit system handoff (`== 0` = armed) |
| `func_8009F5F4` | Opcode 0xA7 `OP_UPDATE_CHARACTER` | **Committed** | `e7b0101` |
| `func_8009EB78` | Opcode 7 actor-script starter | Decoded / fixed | `9e1b667` |
| `func_80080A74` | Actor script slot initialization | **Committed fix** | `9e1b667` |
| `func_80098CAC` | Script-move sprite pointer path | **Committed fix** | `9e1b667` |
| `func_800855C8` | Field sound cue wrapper | Shim (no-op) | `ae8c753` |
| `func_80079288` | Random encounter management | Stub / classified | Post-op54 diagnostic |
| `func_80093B10` | Enable random encounters subroutine | Proven live | Zone 5 trigger |
| `func_800A5C40` | FieldMain transition orchestrator | Committed fix | `4b377be` |
| `func_8001B6C4` | Battle state main | Stubbed | `XENO_KERNEL_SEL=1` |
| `func_801C62A8` | Menu overlay entry | Overlay address | `XENO_KERNEL_SEL=4` harness gap |

## July 9, 2026 additions — animation and child-sprite subsystem

Pinned during the Map1→Map15 reload campaign (see [Phase Log](Phase-Log) and [Field Script VM and Opcodes](Field-Script-VM-and-Opcodes); depth in `docs/ai_context/ACTIVE_HANDOFF.md`, July 9 entries).

| Symbol | Meaning | Status | Evidence source |
|--------|---------|--------|-----------------|
| `g_WorkList` / `g_TimerWorkList` | Render / timer work-list heads, pumped by `WorkListUpdate` / `TimerWorkListUpdate` (per-frame field pump `func_800752C8`) | Verified working | `ce61f3d`; child `ticks=8` in `map15_polycheck_20260709_run3.log` |
| `D_8004FD40` | 16-entry anim render-callback table; retail contents in `3F290.sdata.s`. Port has only `func_80025710`; other retail-non-NULL slots log once (type-2 `func_80025718` = the child visibility gap) | Decoded / trusted | `ce61f3d`; `[port] anim render callback D_8004FD40[2] not ported` in `map1_opcode_e0_reload_20260709_run3_final.log` |
| `D_8004FC40` | 256-byte anim opcode stride table (copied from `3F290.sdata.s`); drives the `func_800248D4`/`func_8001FBE4` advance consulted by every July 9 opcode pass | Committed fix | July 7 `3f469b4` |
| `D_800592E4` / `E8` / `EA` | Opcode `0xFC` VRAM-upload handoff: script-relative blob pointer + VRAM x/y, consumed by `func_8001FB30` → `func_8002DDE4` | Verified working | `ce61f3d`; blob live at VRAM 512,256 (`map1_opcode_fc_reload_20260709_run1.log`) |
| `D_80059310` / `D_80050108` | Texture-page latch from opcode `0x8D` (`func_8002CC10`): `GetTPage(0,0,x,y) & 0x1F` + mode flag `1` ("merge override"); consumed by real temp2.c latch code | Committed fix | `ce61f3d`; `map1_opcode_8d_reload_20260709_run1.log` |
| `func_8007CD3C` | Scratchpad arena push — returns PSX `0x1F800000`-based pointers, **unmapped on the port**; keep push/pop balanced and back the workspace with a C local | Decoded / trusted (port pitfall) | `3858bd8`; live SIGSEGV during the POLYCHECK migration |
| `g_Scene.worldRotationMatrix` (`+0x114`) | World rotation matrix; first factor of the POLYCHECK mode-0 model-matrix composition in `func_80083288` | Decoded / trusted | `include/field/main.h`; `3858bd8` |
| `D_8006FC48` | `"POLYCHECK %d\n"` format string (field rodata); port copy in `pc_port/src/data_field.c` (stubs.c is frozen) | Committed fix | `3858bd8`; `map15_polycheck_20260709_run3.log` |

### `WorkListEntry` layout (0x1C bytes, PSX layout — u32 fields mandatory)

The game embeds these in heap objects at fixed offsets; the port's native-64-bit-pointer version shifted every field and was latent only because the lists had always been empty. Fixed (fields now u32) in `ce61f3d`, including the same latent bug in `func_8001CE74` (opcode `0x96`). Status: Committed fix. Source: `pc_port/src/work_list_port.c`.

| Offset | Field | Meaning |
|--------|-------|---------|
| `+0x0` | `unk0` | Owner `WorkListEntry*` |
| `+0x4` | `unk4` | Payload (`SpriteData*`) |
| `+0x8` | `onTriggerCallback` | Tick/render callback |
| `+0xC` | `onFreeCallback` | Free callback |
| `+0x10` | `unk10` | Own unique id (counter `D_80059184`) |
| `+0x14` | `unk14` | Owner id — the opcode-`0x96` unlink match key; **bit 30** = sticky/skip-unlink (set/cleared by `0xBC` subs `0x24`/`0x25`), **bit 31** = timer flag |
| `+0x18` | `pNext` | Next entry |

### `AnimTask` layout (heap child object, opcode `0xE0`)

`task1` (timer work-list entry) at `+0x0`, `task2` (render work-list entry) at `+0x1C`, `SpriteData` at `+0x38`. Allocated by `func_800233A4`, spawned by `func_80023B84`. Status: Decoded / trusted (`ce61f3d`).

### `SpriteData` offsets learned (child-sprite opcode passes)

Status: Decoded / trusted. Evidence: `ce61f3d`, `11b2474`, `f24e035`, `de23142`, `a389755`, `ae30d53`.

| Offset | Meaning |
|--------|---------|
| `+0x1C` | Gravity (opcode `0xA3` setter; consumed by integrator `func_80022B2C`) |
| `+0x20` | Transform block pointer (rotation-Y at `+0x2` for `0x94`; model-data header latch at `+0x34` for `0xF5`/`0xF6`) |
| `+0x6C` | Wrapper work-list task pointer (sticky-bit target of `0xBC` subs `0x24`/`0x25`) |
| `+0x70` | Parent sprite back-link (`0x94` inherit-rotation; `0xBC` sub `0x16` move-to-parent) |
| `+0x7C` | Shared block pointer (`0xC6` conditional store at `+0xC`; gravity fallback at `+0x4` for `0xA3`) |
| `+0xA0/A2/A4` | Target-position halfwords (`0xBC` shared tail, op0 bit 6 set) |

## Actor indices (Map1, documented)

| Actor | Role |
|-------|------|
| 1 | Fei (player) |
| 18 | Exit transition target actor (op7 routine 4) |
| 20 | Owner of shared `var0x0408` door state |
| 48 | Exit zone poll routine (var `0x0462` latch) |

## Flag words (actor script slots)

Documented from op7 slot diagnostic:

- Idle/free: `0x003cffff` → priority `0xF`, busy `0`
- Buggy init: `0xffff0000` → priority `0xF`, busy `1` (blocked op7 before fix)
- Active scheduler: `0xffdf0000` → priority `7`, busy `1`

## PSX vs host type pitfalls

| PSX type | Host pitfall | Example |
|----------|--------------|---------|
| `u_long` (4 bytes) | Host `u_long` often 8 bytes | `FieldScene.unk7C` layout drift |
| `rand()` | PSX `0..32767` vs glibc large range | Actor 20 `var0x0408` chooser |
| Structs embedded in PSX heap objects | Native 64-bit pointer fields shift every offset (48 B vs 0x1C) | `WorkListEntry` garbage-callback SIGSEGV, fixed `ce61f3d` |
| PSX scratchpad pointers (`0x1F800000`) | Unmapped on host — dereference SIGSEGVs | `func_8007CD3C` workspace convention, `3858bd8` |
