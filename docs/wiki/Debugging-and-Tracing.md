# Debugging and Tracing

Practical gdb/logging commands from documented handoff workflows. Logs are stored under `captures/render_diag/` (gitignored locally).

## Prerequisites

- Build with debug symbols: `./pc_port/build_port.sh` (uses `-g -O0`)
- Run inside `distrobox enter xenogears-dev`
- For **any** run that must reach the field, **`XENO_KERNEL_SEL=0` is mandatory** — without it `PcPort_ForcedKernelSelect` is a no-op and the process spins at the kernel menu forever. Documented harness lesson (July 9): this menu-spin was once misread as a host stall, and it still exits `RC=124`, so a timeout alone does not prove the field was reached. With it set, cold-boot `FieldLoad` is ~6 s and the full Map1→Map15 reload ~10 s.
- `rg` is **not** available in container — use `grep -E`

## Build integrity and compile-time diagnostics

Always inspect the build's `compiled=` / `skipped=` line. The current port
driver suppresses game-TU compiler errors, skips failed units, and may let
stubs stand in for their symbols; `LINK OK` alone is therefore insufficient.
See [`OPEN_ISSUES.md`](../../OPEN_ISSUES.md) before treating a runtime result as
evidence from full game code.

Compile a gated diagnostic without editing `GFLAGS`:

```bash
XENO_DIAG_DEFINES=-DXENO_DIAG_OPCODE_SWEEP ./scratchpad/run_build_port.sh
```

For the passive animation-opcode trace, use the launcher rather than passing
map variables or mixing binary records into stderr:

```bash
./scratchpad/run_map001.sh --opcode-sweep /tmp/map001-opcodes.bin 10
```

The fixed records are 16 bytes. A final little-endian `END!` tag means the
launcher regained control after the watchdog; an absent tag makes the run
uninterpretable as a completed survey.

## Basic harness runs

### Map0 stability check

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 timeout -s KILL 45 ./pc_port/build_native/xeno-port'
```

### Map1 with diagnostics

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 \
  XENO_FIELD_ENTRANCE=9 XENO_FIELD_0BB_VRAM_UPLOAD=1 \
  XENO_FIELD_DIAG=1 timeout -s KILL 90 ./pc_port/build_native/xeno-port'
```

### Filtered log (stub / frame / assert events)

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp/pc_port && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=9 \
  XENO_FIELD_0BB_VRAM_UPLOAD=1 timeout -s KILL 90 build_native/xeno-port 2>&1 | \
  grep -E "RUN_RC|\[stub\]|func_8001E3D8|frame=|assert|SIG|Aborted|INSIDE3D|OP7_" | \
  tee ../captures/render_diag/map1_filtered_$(date +%Y%m%d_%H%M%S).log; \
  echo RUN_RC=${PIPESTATUS[0]}'
```

## Return code reference

| RC | Meaning |
|----|---------|
| 124 | `timeout` clean kill (success for harness) |
| 134 | SIGABRT (assert) |
| 137 | SIGKILL from `timeout -s KILL` |
| 139 | SIGSEGV |

## Gdb — field scene / projection

Documented probes from visual recovery investigation:

```gdb
# Break after FieldScene layout fix
break func_80024FF4
commands
  print &g_Scene.worldToScreenMatrix
  print (void*)((char*)&g_Scene + 0xD4)
  continue
end

# First actor quad projection
break RotTransPers4
commands
  print $C2_H
  print g_Scene.sceneScrZ
  continue
end

# Actor packet OT link
break ParsePrimitive
condition 1 polyTag->code == 0x2d
```

## Gdb — exit trigger zone 11 route

Documented synthetic input pattern:

1. Set env: `XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=9 XENO_FIELD_0BB_VRAM_UPLOAD=1 XENO_KERNEL_SEL=0`
2. Inject `D_800AFE9C=0x1000` (+X) frames 1–105
3. Then `D_800AFE9C=0x2000` (+Z)
4. Break `misc11.c:992` for inside-3D trigger
5. Break `func_8009EB78` for opcode 7 slot allocation

Example gdb script location: `captures/render_diag/map1_exit_zone11_route_ent9_20260708.gdb`

## Gdb — opcode 7 slot state

Watch actor 18 slot flags after `func_80080A74`:

```gdb
break func_80080A74
commands
  bt
  continue
end

break func_8009EB78
commands
  printf "OP7_START_SCRIPT\n"
  continue
end
```

Expected after fix (`9e1b667`): slots initialize to `0x003cffff`; op7 advances IP.

## Gdb — var 0x0408 writer trace

Lightweight approach (heavy global watchpoints timed out in documented probes):

- Break `FieldScriptVMHandlerVariableAssign` / mul-with-rand handlers
- Log actor index + IP + value written
- Reference log: `captures/render_diag/map1_actor18_var0408_init_probe_20260708_150922.log`

## Gdb — zone-5 one-shot reload probe (Map1 → Map15)

The July 9 campaign's standard probe: the probe gdb-injects synthetic d-pad input (`D_800AFE9C = 0x2000`, +Z) at `misc2.c:1688`, gated to pre-reload frames; Fei walks into Map1's zone-5 trigger, whose script chain (the A50 script) issues `CHANGE_FIELD` (op 152) through the retail path — the transition itself is not injected. The probe then follows the live reload into Map15 and counts every milestone. Script: `captures/render_diag/map1_opcode_e0_reload_20260709.gdb` — reused unchanged for every opcode pass from `0x8D` through `0xBC` sub `0x25`.

What it does:

- Breaks `misc2.c:1688` every frame; injects `D_800AFE9C=0x2000` (+Z) **only while `FieldLoad` hits < 2**, so synthetic input stops once the reload starts
- Counters: `FieldLoad` hits, `FieldTextBoxInitialize` / `FieldFadeInitialize` (reload-gated), child spawns (`func_80023B84`), child ticks (`func_80022DF4`), child frees (`func_80022EB8`)
- `catch signal SIGABRT` / `catch signal SIGSEGV` → prints all counters + `bt 16`, then quits
- Prints `POST_RELOAD_OK` and exits clean if the run survives to frame ≥ 200 post-reload

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  topic=my_topic; \
  SDL_VIDEODRIVER=x11 DISPLAY=:0 \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=8 \
  timeout -s KILL 180 gdb -batch -x captures/render_diag/map1_opcode_e0_reload_20260709.gdb \
    ./pc_port/build_native/xeno-port 2>&1 | \
  tee captures/render_diag/${topic}_reload_$(date +%Y%m%d_%H%M%S).log'
```

Healthy-run milestones (state as of `3442f3f`): `FIELDLOAD #2 frame=116 map=15` → `RELOAD_TEXTBOX` / `RELOAD_FADE` → `SPAWN #1..#3` (type-2 children, `hdr=0200`) → `ticks` climbing (8 by the frame-117 pump) → the next `SIGABRT` backtrace names the current frontier opcode. Evidence logs: `map1_opcode_{fc,e0,8d,f5,a3,bc,94}_reload_20260709*.log`, `map15_polycheck_20260709_run3.log`, `map15_bc25_20260709_run1.log`.

Probe gotchas (documented July 9):

- Compound `break FUNC if $a && $b` conditions can silently never fire in batch mode — put a simple condition on the `break` line and move compound logic into the `commands` `if` block. This exact artifact produced the false "`FieldLoad` hangs in its first ~35 lines" conclusion.
- Conditional breaks on hot functions (e.g. `break HeapAlloc if allocSize==171056`) force a stop/evaluate/resume cycle on every call and can starve the run — prefer unconditional breaks on rarely-called functions.

## Gdb — targeted anim-opcode operand capture

One-shot capture of live operand bytes for a specific anim-script opcode, before or after implementing it. Add to the zone-5 probe above (before its `run` line), with `N` = the opcode byte:

```gdb
# N = opcode byte, e.g. 0xBC
break func_8001FBE4 if opcodeIndex == 0xBC
commands
  silent
  printf "OPC 0x%02x sprite=%p operands=%p op0=%02x op1=%02x op2=%02x sub=0x%02x\n", \
    opcodeIndex, pSpriteData, operands, \
    ((unsigned char*)operands)[0], ((unsigned char*)operands)[1], \
    ((unsigned char*)operands)[2], ((unsigned char*)operands)[0] & 0x3f
  bt 8
  quit
end
```

Change `quit` to `continue` to log every hit instead of the first. For `0xBC`, `op0` bit 7 selects the sub-dispatch, bit 6 the target-position vs position store, and `op0 & 0x3f` is the sub-command (live example: `op0=0xA4` → sub `0x24`). See [Field-Script-VM-and-Opcodes](Field-Script-VM-and-Opcodes) for the decoded chain.

## Smoke triple (ent8 / ent0 / Map0)

Standard no-regression gate after every pass: three plain 30-second runs. Pass = all `RC=124`, stub lines equal to the documented baseline family (`func_80028B14`, `func_80072254`, `func_8008CD48`, `func_8008D0F4`, `func_8009E91C` + Map0's own), zero asserts, zero new `[port]` lines.

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  topic=my_topic; ts=$(date +%Y%m%d_%H%M%S); \
  for cfg in "ent8|XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=8" \
             "ent0|XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=0" \
             "map0|"; do \
    name=${cfg%%|*}; extra=${cfg#*|}; \
    log=captures/render_diag/${topic}_smoke_${name}_${ts}.log; \
    env XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 $extra \
      timeout -s KILL 30 ./pc_port/build_native/xeno-port > "$log" 2>&1; \
    echo "$name RC=$?"; \
    grep -E "\[stub\]|\[port\]|Assertion|assert|SIG|field-diag" "$log" | sort -u; \
  done'
```

A meaningful field-reaching smoke shows exactly one `[field-diag] actors allocated` line per run — with `XENO_KERNEL_SEL=0` missing, `RC=124` passes vacuously from the menu-spin (see Prerequisites).

## Stub oracle

Any `[stub] <symbol>` line in port output = next missing function on the live path.

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 timeout -s KILL 90 ./pc_port/build_native/xeno-port 2>&1 | \
  grep "\[stub\]" | sort -u'
```

Current generated stub count: **697**, unchanged across the entire July 9 opcode chain (handoff, July 9 entries). Soft stubs newly *reached* on the map15 path (log-once, non-blocking): load path `func_8001B5E8`, `SoundFreeWdsEntry`, `func_8008E718`; frame 117 `func_80097954`, `func_8003A450`, `func_8008FB98`.

## Menu-arc gdb traps

Two environment-specific gotchas from the nav arc. Both cost real time to rediscover.

**`g_Menu` must be read through an explicit cast.** In the port `g_Menu` is a
`u8[15664]` stub symbol, so the type gdb picks for a bare `g_Menu->...` expression is
**build-dependent** and can silently resolve wrong. Always go through the pointer type:

```gdb
set $menu = *(SystemMenu**)&g_Menu
printf "choice=%d input=%d\n", (int)$menu->menu1Choice, (int)$menu->input
```

**Inferior-call machinery crashes here.** Calling into the process from breakpoint
command lists — `glReadPixels` in particular — crashes under this environment. Use the
lean counter/readback pattern instead: break, increment a convenience variable, print
scalars or `dump binary memory`, `continue`. No inferior function calls.

```gdb
break func_801E5058
commands
  silent
  set $hits = $hits + 1
  eval "dump binary memory /tmp/gs_before.bin (char*)&g_GameState (char*)&g_GameState + 0x22B8"
  continue
end
```

**`finish` does not resume inside a `commands` block.** To capture state *after* a call
returns, set a second breakpoint on the post-call source line rather than using `finish`.

## What NOT to treat as signal

| Run config | Why invalid |
|------------|-------------|
| Host without distrobox / missing SDL | Toolchain/libs missing |
| `XENO_KERNEL_SEL=4` without overlay | Hits `func_801C62A8` stub — harness gap |
| `XENO_KERNEL_SEL=1` | Battle main stubbed |
| Dummy video without X11 for input milestone | Real keyboard milestone needs `SDL_VIDEODRIVER=x11` |
| Entrances 6 / 10 on Map1 | Harness-invalid; crash |

## Evidence file naming

Handoff convention: `captures/render_diag/<topic>_<YYYYMMDD>_<HHMMSS>.log`

Keep gdb scripts alongside: `captures/render_diag/<topic>_<YYYYMMDD>.gdb`
