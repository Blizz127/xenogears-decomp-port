# Useful Commands

Copy-paste commands from documented handoff and `pc_port/` notes only.

## Build

```bash
# PC port (primary workflow)
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && ./pc_port/build_port.sh'

# Matching decomp (separate toolchain container)
make -B build
```

## Run — field test harness

```bash
# Map0 kernel0 field route (45s)
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 timeout -s KILL 45 ./pc_port/build_native/xeno-port'

# Map1 visible control milestone (real keyboard)
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  SDL_VIDEODRIVER=x11 XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 \
  XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=0 XENO_FIELD_0BB_VRAM_UPLOAD=1 \
  ./pc_port/build_native/xeno-port'

# Map1 entrance 9 (July 8 exit-route probes — historical)
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  SDL_VIDEODRIVER=x11 XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 \
  XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=9 XENO_FIELD_0BB_VRAM_UPLOAD=1 \
  timeout -s KILL 90 ./pc_port/build_native/xeno-port'
```

## Run — current standard (July 9): zone-5 reload probe + smoke triple

See [Debugging and Tracing](Debugging-and-Tracing) for the probe internals.

```bash
# Zone-5 reload probe run (entrance 8, gdb-injected +Z d-pad → A50 CHANGE_FIELD)
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && SDL_VIDEODRIVER=x11 DISPLAY=:0 XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=8 timeout -s KILL 240 gdb -batch -x captures/render_diag/map1_opcode_e0_reload_20260709.gdb ./pc_port/build_native/xeno-port'
```

```bash
# Smoke triple (ent8 / ent0 / Map0 — 30s plain runs, expect baseline stub family only)
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=8 \
  timeout -s KILL 30 ./pc_port/build_native/xeno-port 2>&1 | \
  grep -E "\[stub\]|\[port\]|assert|SIG|Abort"'

distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=0 \
  timeout -s KILL 30 ./pc_port/build_native/xeno-port 2>&1 | \
  grep -E "\[stub\]|\[port\]|assert|SIG|Abort"'

distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 \
  timeout -s KILL 30 ./pc_port/build_native/xeno-port 2>&1 | \
  grep -E "\[stub\]|\[port\]|assert|SIG|Abort"'
```

## Run — with logging

```bash
# Tee full log
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp/pc_port && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=9 \
  XENO_FIELD_0BB_VRAM_UPLOAD=1 timeout -s KILL 90 build_native/xeno-port 2>&1 | \
  tee ../captures/render_diag/run_$(date +%Y%m%d_%H%M%S).log; echo RUN_RC=${PIPESTATUS[0]}'

# Opt-in field diagnostics
XENO_FIELD_DIAG=1 ...

# Filter stubs only
... 2>&1 | grep "\[stub\]"
```

## Menu nav harness (`XENO_MENU_NAV_TEST`)

The menu/nav arc's regression suite. Every mode boots the field, forces the main menu
open, and drives synthetic pad edges at the **raw BIOS pad buffer** (`g_C1Buffer`) —
exactly where a real keypress lands — so `ControllerPoll` derives genuine rising edges.

Injection is clocked off **menu reader ticks**, not Vsync frames: the field's open/close
phases reset the pad queue at a different cadence, so frame counting races it.

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  env XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=5 XENO_FIELD_ENTRANCE=0 \
      XENO_MENU_FORCE=1 XENO_MENU_NAV_TEST=items \
      timeout -s KILL 200 pc_port/build_native/xeno-port'
```

| Mode | Drives | Asserts on |
|------|--------|-----------|
| `items` | Open Items, cancel, close | Windows 3/4 settle open; teardown; nav alive after close |
| `items-reorder` | Select row 0, RIGHT to row 1, confirm | `row0/row1` swap via `g_GameState` readback |
| `items-prompt` | Confirm row 0 twice, hold, cancel | Prompt opens with a default target; state `unchanged=yes` |
| `prompt-nav` | DOWN x3 then UP x3 in the prompt | Wraparound both directions; state `unchanged=yes` |
| `prompt-nav-skip` | DOWN x2 on a **two-member** party | Ineligible-target skip; state `unchanged=yes` |
| `item-use` | Confirm an ordinary item on the default target | `hp 20/50 → 50/50`, `qty 1→0`, ID cleared, chime `0x37` |
| `item-special` | Confirm a **magnitude-1** special (item ID 33) | Five inventory families populated, IDs sequential at qty 10 |

**Locked-schedule modes are the binding regression gate:** `items`, `items-reorder`,
`items-prompt`, `item-use` and `item-special` run on a fixed tick schedule and their
markers must reproduce exactly. The longer prompt-nav runs have more animation phase in
them, so their *closed-frame counts* can legitimately differ between runs — compare the
state markers, not the frame numbers.

> **One seed per mode.** Each mode owns its own diagnostic seed (`prompt-nav-skip`
> reshapes the party; `item-use` sets Fei to 20/50 and forces quantity 1; `item-special`
> plants the magnitude-1 special at row 0). A new test seed gets a **new mode** — never
> add a seed to an existing one, or every baseline captured under it becomes invalid.

> **Magnitude matters for `item-special`:** item ID 33 is magnitude-1 and routes to the
> ported `func_801E5058`. **ID 34 is magnitude-2** and would route into the parked
> `func_801E5178`.

## Matching ROM checksum gate

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && make rom-check'
```

Four FAILs (`slus_006.64`, `field.bin`, `member_change_menu.bin`, `shop_menu.bin`) is the
correct result today. See [Build and Run](Build-and-Run) for what the gate guarantees and
why the pins must never be re-pinned.

## Kernel routing probes

```bash
# Battle route (expect immediate func_8001B6C4 stub)
XENO_FIELD_TEST=1 XENO_KERNEL_SEL=1 timeout -s KILL 30 ./pc_port/build_native/xeno-port

# Menu route (expect func_801C62A8 stub — harness gap)
XENO_KERNEL_SEL=4 timeout -s KILL 30 ./pc_port/build_native/xeno-port
```

## Git (host only)

```bash
cd /home/blizz/Projects/xenogears-decomp
git status
git log --oneline -10
```

## Wiki sync preview

```bash
./scripts/preview_wiki_sync.sh ~/Projects/xenogears-decomp-ai.wiki
```

## Wiki manual sync

```bash
git clone git@github.com:Blizz127/xenogears-decomp-ai.wiki.git

rsync -av \
  ~/Projects/xenogears-decomp/docs/wiki/*.md \
  ~/Projects/xenogears-decomp-ai.wiki/

cd ~/Projects/xenogears-decomp-ai.wiki
git status
git add *.md
git commit -m "Update Xenogears project wiki"
git push
```

## Phase 1 slice (from pc_port/README.md)

```bash
# After matching ELFs exist
gcc -c src/field/game_logic/gold.c -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
    -include assert.h -w \
    -I pc_port/include_shim -I include \
    -I pc_port/extern/PsyCross/include -I pc_port/extern/PsyCross/include/psx \
    -o gold.o
nm -u gold.o | awk '{print $2}' | sort -u > undef.txt
python3 tools/scripts/gen_port_stubs.py \
    --elf build/out/slus_006.64.elf --elf build/out/field.elf \
    --undefined undef.txt --out pc_port/generated/stubs.c
```

## Environment variable quick reference

```bash
export XENO_FIELD_TEST=1
export XENO_KERNEL_SEL=0          # 0=field, 1=battle, 4=menu
export XENO_FIELD_MAP=1
export XENO_FIELD_ENTRANCE=9      # 0, 8, 9 documented; NOT 6 or 10
export XENO_FIELD_0BB_VRAM_UPLOAD=1
export XENO_FIELD_DIAG=1          # optional verbose field prints
export SDL_VIDEODRIVER=x11        # real keyboard input
```

## Container notes

```bash
distrobox enter xenogears-dev     # toolchain + SDL/OpenAL
# git NOT on PATH in container — use host for git commands
# grep -E instead of rg in container
```
