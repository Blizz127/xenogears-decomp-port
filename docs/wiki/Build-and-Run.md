# Build and Run

Summarized from [`pc_port/README.md`](https://github.com/Blizz127/xenogears-decomp-ai/blob/main/pc_port/README.md), [`pc_port/build_port.sh`](https://github.com/Blizz127/xenogears-decomp-ai/blob/main/pc_port/build_port.sh), and [`ACTIVE_HANDOFF.md`](https://github.com/Blizz127/xenogears-decomp-ai/blob/main/docs/ai_context/ACTIVE_HANDOFF.md).

## Environment

- **Dev container:** `distrobox enter xenogears-dev` (documented workflow on Bazzite/immutable host).
- **Toolchain packages:** `gcc`, `g++`, `cmake`, `make`, `pkg-config`, `binutils`, `python3`, `libsdl2-dev`, `libopenal-dev`, `libgl1-mesa-dev`.
- **PsyCross:** vendored at `pc_port/extern/PsyCross` (gitignored). Clone if missing:

  ```bash
  git clone --depth 1 https://github.com/OpenDriver2/PsyCross.git pc_port/extern/PsyCross
  ```

- **Matching ELFs** (optional, for stub symbol classification): build with `make -B build` in the matching (gears/ninja) environment.

> `git` is **not** on PATH inside the distrobox container. Run git commands on the **host** at `/home/blizz/Projects/xenogears-decomp`.

## Build PC port

From repo root inside `xenogears-dev`:

```bash
./pc_port/build_port.sh
```

Output: `pc_port/build_native/xeno-port`

Alternative CMake path (from `pc_port/README.md`):

```bash
cd pc_port
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
./build/xeno-port
```

The documented day-to-day workflow uses `build_port.sh` → `build_native/xeno-port`.

## Run — default field test (Map 0)

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 timeout -s KILL 45 ./pc_port/build_native/xeno-port'
```

- `XENO_FIELD_TEST=1` — enter field test harness (skips full boot).
- `XENO_KERNEL_SEL=0` — force kernel menu to select field state (`FieldMain`).
- `RUN_RC=124` — timeout success (no crash); only meaningful with `XENO_KERNEL_SEL=0` set; without it the kernel menu spins forever and still exits RC=124 (see [Debugging and Tracing](Debugging-and-Tracing)).

**Known issue:** default Map0 route (no `XENO_FIELD_MAP`) may render **black**. This is pre-existing and separate from Map1 milestones.

## Run — Map 1 with visible player control

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && \
  SDL_VIDEODRIVER=x11 XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 \
  XENO_FIELD_MAP=1 XENO_FIELD_ENTRANCE=0 XENO_FIELD_0BB_VRAM_UPLOAD=1 \
  ./pc_port/build_native/xeno-port'
```

Requires a live X display (`DISPLAY=:0` on desktop, or Xvfb for headless synthetic input).

## Documented environment variables

| Variable | Purpose | Documented in |
|----------|---------|---------------|
| `XENO_FIELD_TEST=1` | Enable field test entry path | Handoff, `port_main.c` |
| `XENO_KERNEL_SEL=<n>` | Force kernel state: `0`=field, `1`=battle, `4`=menu | Handoff |
| `XENO_KERNEL_DELAY` | Frames before forced selection (default 60) | Handoff |
| `XENO_FIELD_MAP=<n>` | Set field map via `D_8006F94E` | Handoff, `port_main.c` |
| `XENO_FIELD_ENTRANCE=<n>` | Set spawn entrance via `D_8006F954` | Handoff, `port_main.c` |
| `XENO_FIELD_0BB_VRAM_UPLOAD=1` | Opt-in synchronous `0xBB` VRAM upload | Handoff, `archive_port.c` |
| `XENO_FIELD_DIAG=1` | Re-enable bounded `[field-diag]` printfs | Handoff |
| `SDL_VIDEODRIVER=x11` | Use X11 for real keyboard input | Handoff milestone demo |

### Valid Map1 entrances (documented)

- **0, 8, 9** — used in verified repros.
- **Do not use 6 or 10** — out-of-range spawn-table walkmeshId; crash.

### Synthetic d-pad injection (gdb diagnostics)

For exit-route probes, documented direction map on `D_800AFE9C`:

| Value | Direction |
|-------|-----------|
| `0x1000` | +X |
| `0x2000` | +Z |
| `0x4000` | −X |
| `0x8000` | −Z |

## Matching decomp build (separate from PC port)

From repo root in the ethos/matching container:

```bash
make -B build
```

See root [`README.md`](https://github.com/Blizz127/xenogears-decomp-ai/blob/main/README.md) for decomp progress on [decomp.dev](https://decomp.dev/ladysilverberg/xenogears-decomp).

## Full-ROM checksum gate — `make rom-check`

Added in `57db1d9`. Runs `tools/scripts/check_rom_hashes.sh`.

```bash
distrobox enter xenogears-dev -- bash -lc 'cd /home/blizz/Projects/xenogears-decomp && make rom-check'
```

What it does, and why each part matters:

- **Always clean by design.** It runs `rm -rf build linker` then `make build` — the
  documented flow, which includes the Makefile's `ApplyMatrixSV` sed at
  `Makefile:73-74`. A dirty tree is exactly how false greens happen.
- **Protects the objdiff baseline.** `make build` begins with `make clean`, which would
  delete `expected/`. The script moves it aside and restores it under an `EXIT` trap.
- **Reports per-overlay PASS/FAIL** against `config/checksum.sha`, printing built-vs-pin
  hashes and sizes, plus a known-red ledger line under each expected failure. Exits
  nonzero when anything fails.
- **Never writes `config/checksum.sha`.** The pins are retail ground truth. A FAIL means
  the *build* drifted from retail — never that the pin is wrong. **Do not re-pin.**
- **`menu.bin` is informational only** — a WIP overlay, deliberately not pinned.

> **Four FAILs is the correct result today.** `slus_006.64`, `field.bin`,
> `member_change_menu.bin`, and `shop_menu.bin` are known-red source regressions with
> named causes — see the nav-arc entry in [`OPEN_ISSUES.md`](../../OPEN_ISSUES.md). This
> is not a green build and must not be reported as one.

**False-green resistance is a tested property:** planting byte-perfect retail files as
`build/out/*` passes a naive `sha256sum --check`; `make rom-check` wipes, rebuilds, and
still reports the same four honest FAILs. If a fast advisory mode is ever added it MUST
be a separate flag that refuses to print PASS.

Cost is a few minutes per invocation. That is accepted — it is a deliberate
before/after-matching-changes check, not a per-edit gate.

## Environment traps

Each of these has cost real debugging time. They are cheap to avoid and expensive to
rediscover.

| Trap | Symptom | Rule |
|------|---------|------|
| `gears` locates the project by the **literal** directory name `xenogears-decomp` | Tooling silently fails or targets the wrong tree | Keep clones/worktrees named `xenogears-decomp` |
| A bare `build/` **directory** shadows make's `build` **target** | `make build` appears to do nothing | Never leave a stray `build/` in a state where the target is needed |
| The main flow applies an `ApplyMatrixSV` sed (`Makefile:73-74`) that raw `ninja` lacks | Raw-ninja builds diverge from `make build` output | Build via `make build`, not bare `ninja`, when hashes matter |
| Checkout-hopping without `rm -rf build linker` | **Convincing false greens** — bit the pin bisect twice | Always wipe `build linker` after changing checkout |
| `git` is not on PATH in the container | git commands fail inside distrobox | Run git on the **host** |

## Success signals

| Signal | Meaning |
|--------|---------|
| `LINK OK -> pc_port/build_native/xeno-port` | Build succeeded |
| `RUN_RC=124` | Timed out cleanly (expected for harness runs) — only meaningful with `XENO_KERNEL_SEL=0` set; without it the kernel menu spins forever and still exits RC=124 (see [Debugging and Tracing](Debugging-and-Tracing)) |
| `RUN_RC=134` / `139` | SIGABRT / SIGSEGV — investigate backtrace |
| Zero `[stub]` lines | No missing functions hit on that route (bounded window) |
| `[stub] <name>` | Next boot-path oracle target |
