# Xenogears PC Port (experimental, Shipwright-style)

A native PC source port built **on top of the matching decompilation**, in the
spirit of [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) for
Zelda OoT and the [Silent Hill PC port](https://github.com/SlickAmogus/silent-hill-decomp)
for PSX. This directory is **separate from the matching (gears/ninja/MIPS)
build** and does not affect it.

## Architecture

```
  Xenogears game logic (../src/*)         <- decompiled C
            │  calls the PsyQ SDK (DrawOTag, RotTransPers, Spu*, CdRead, ...)
            ▼
  PsyCross  (PSX hardware abstraction layer)
    GTE in software · LibGPU -> OpenGL · LibSPU -> OpenAL ·
    LibCD -> BIN/CUE · controllers -> SDL2
```

PsyCross is the PSX equivalent of libultraship (the N64 HAL behind SoH). The
port compiles the game against PsyCross's PsyQ-compatible headers
(`extern/PsyCross/include/psx`) and links its implementation, **instead of** the
on-hardware reimplementations in `../src/slus_006.64/psyq/*`.

## Status

- [x] **Phase 0 (scaffold):** PsyCross vendored (`extern/PsyCross`, gitignored)
      and building from source; `xeno-port` links against it and brings the
      runtime up/down.
- [x] **Phase 1 pattern (proven on a slice):** a real game translation unit
      (`src/field/game_logic/gold.c`) compiles natively in port mode, its
      undefined symbols are auto-stubbed, and the linked binary **runs real
      decompiled game logic on x86**, with the stub oracle logging the next
      function the code path needs. See "Phase 1 loop" below.
- [ ] **Phase 1 (scale-up):** compile *all* game TUs, reconcile the full PsyQ
      header surface, link the whole executable, hand `main` off to the game.
- [ ] **Phase 2:** asset/disc extraction pipeline feeding PsyCross's LibCD.
- [ ] **Phase 3:** climb the boot path, replacing stubs with decompiled C until
      the intro/field renders.

> A native port requires every executed function to be real C — raw `INCLUDE_ASM`
> (MIPS) cannot run on x86. Port progress is therefore gated on functional
> decompilation coverage; byte-for-byte matching is **not** required for the port.

## The Phase 1 loop (boot-path-driven decompilation)

The port doubles as the decompilation priority oracle:

1. Compile game TUs in port mode (`-DXENO_PC_PORT -DSKIP_ASM`, against the
   `include_shim/` -> PsyCross headers). `INCLUDE_ASM` becomes inert.
2. Collect undefined symbols: `nm -u *.o`.
3. `tools/scripts/gen_port_stubs.py` classifies each (function vs data, via the
   matching-build ELF symbol tables) and emits logging stubs + zeroed storage.
4. Link the executable and run it. It runs real decompiled code until it hits a
   stub on the live path, which logs `[stub] <name>`.
5. Decompile that function (matching optional), rebuild, repeat. Each step
   extends how far the game runs — and grows the matching % at the same time.

Reproduce the proven slice (Linux, in a container/distrobox with `gcc`,
`libc6-dev`, `binutils`, `python3`):

```bash
# from repo root; build the matching ELFs first (for symbol classification)
#   (in the ethos container)  make -B build

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

## Game data and Sony code

The port ships no game data and no Sony Psy-Q code.

- **Game data comes from your disc at run time.** Put your Xenogears (USA,
  SLUS-00664) files in `disc/` (git-ignored): the Disc 1 image
  (`disc1.bin` + `disc1.cue`, or `XENO_DISC=`) and the files the matching build
  extracts (`SLUS_006.64`, `field.bin`, `menu.bin`, `shop_menu.bin`,
  `member_change_menu.bin`, `world_map.bin`; or `XENO_DATA_DIR=`). Native
  tables in `src/data_*.c` are zero storage plus (file, offset, size) rows;
  `src/retail_data.h` fills them before `main()` after checking each file
  against `config/checksum.sha`. On first run `src/disc_check.c` checks the
  disc image against the Redump dump (redump.org/disc/177) and remembers the
  result in `~/.cache/xenogears-port/`. A missing or wrong file stops the
  port with an explanation.
- **No retail bytes in git.** `tools/analysis/retail_data_guard.py` fails when
  a tracked file carries retail bytes (compared with your `disc/` files, or
  with committed one-way hash signatures in CI).
- **No Psy-Q in the port.** The PSX layer is PsyCross (MIT, pinned; see
  `THIRD_PARTY.md`) plus `src/psyq_compat*.c`. `tools/analysis/psyq_port_guard.py`
  fails on Psy-Q functions/tables in `pc_port/`, and `build_port.sh` runs it
  on the linked binary so no matching-build Psy-Q TU can be linked.
- Run all checks with `make port-guards`; install
  `tools/analysis/pre-commit-port-guards` as a git pre-commit hook (or use
  `.pre-commit-config.yaml`). CI runs them in `.github/workflows/port-guards.yaml`.

## Port layers, config and mods

Full description: [`docs/port/ARCHITECTURE.md`](../docs/port/ARCHITECTURE.md).

Port code reaches the platform through interface headers in
`include/xg_plat/` (the moddable-port pattern of the Parasite Eve port's
`docs/ARCHITECTURE-PORT.md`), each with a swappable backend in `src/plat/`:

| Interface | Covers | Backend now |
|---|---|---|
| `disc.h` | disc image, retail files, BIOS, sector reads, verification | stdio + `retail_data.h` + `disc_check.c` (weak, overridable) |
| `renderer.h` | window/output size, fullscreen, vsync, filtering, widescreen query | PsyCross |
| `input.h` | keyboard bindings by SDL key name | PsyCross |
| `timing.h` | game speed, fast-forward, vblank clock | PsyCross |
| `audio.h` | user volume | none yet (reported unsupported) |
| `config.h` | `config.ini`, `--config`/`--set`, `XENO_*` overrides | `src/plat/xg_plat_config.c` |
| `mods.h` | `mods/` folders, events, C plugins, asset replacement | `src/plat/xg_plat_mods.c` |

The game's own PSX calls still go through PsyCross + `psyq_compat*`; areas
move behind the interfaces as they are touched.

**Config:** copy `config.example.ini` to `config.ini` next to the executable or
to `~/.config/xenogears-port/`, or pass `--config FILE`; `--set key=value` and
`XENO_<SECTION>_<KEY>` override single keys. Widescreen, 60 fps and user volume are accepted but
reported as unsupported until a backend implements them.

**Mods:** each folder in `mods/` (or `~/.local/share/xenogears-port/mods/`)
has a `mod.txt` (`name`, `version`, `load_order`, `abi = 1`), optional
`assets/<sha256>.bin` replacements keyed by the SHA-256 of the original bytes
(archive files, streamed files, `LoadImage` texture blocks, `SpuWrite` sample
blocks), and an optional `plugin.so` exporting `xg_mod_init()`. Events: `boot`,
`frame_tick`, `asset_loaded`, `room_enter`. `XENO_MODS_LOG_ASSETS=1` prints the hash of each
file the game loads; `XENO_MODS_DUMP_DIR=dir` saves the originals from your
disc for editing. Mods are user content: `mods/` is git-ignored and must never
be committed. Full guide: [docs/port/MODDING.md](../docs/port/MODDING.md).

**Cheats:** `god`, `encounters`, `save`/`load`, `warp`, `battle`, `speed` are
cheat-console commands (`src/cheat_console.c`) run on `frame_tick`; the host
toolbar and F10/F12 queue them, and `cheats.console = on` reads them from stdin.

## Building (Linux)

The port runs from the game discs without a Sony BIOS. Memory-card titles and
field glyph strips use the bundled public-domain Jiskan 16 bitmap font, mapped
from Shift-JIS codes to the same 16x16 KROM glyph shape. Its pixels can differ
from Sony's font; details and source are in `font_data/README.md`. Setting
`XENO_BIOS` explicitly opts into the SCPH-5500 KROM path, which requires the
user's 512 KB BIOS with SHA-256
`11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef`.
The verified copy is retained for the process lifetime. An explicitly chosen
but invalid BIOS stops with `KROM_UNRESOLVED`.

Requires `cmake`, a C/C++ compiler, and dev packages for **SDL2**, **OpenAL**,
and **OpenGL**. The full `build_port.sh` driver also requires **OpenSSL 3**
development files (`libssl-dev` on Ubuntu) for the optional `XENO_BIOS` path.
The BIOS payload is not embedded in the build. On the immutable Bazzite host, build inside a container or a
`distrobox` (Ubuntu shown):

```bash
# from repo root
# PsyCross is pinned in pc_port/psycross.lock (MIT, see pc_port/THIRD_PARTY.md);
# pc_port/build_port.sh clones it at that commit when absent. Manually:
git clone https://github.com/OpenDriver2/PsyCross.git pc_port/extern/PsyCross
git -C pc_port/extern/PsyCross checkout e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6

cd pc_port
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
./build/xeno-port      # opens a window on a real display
```

## Host hotkeys

- A clickable host toolbar is reserved above the game viewport. Its `SAVE`,
  `LOAD`, and `RECORD` buttons perform the same actions as the hotkeys below.
  Save/load displays amber `WAIT` while field control is locked, green
  `SAVED`/`LOADED` on success, or red `ERROR` on failure. While recording,
  `RECORD` becomes a red `STOP` button. The toolbar is drawn after capture
  readback and is therefore not included in screenshots or MP4s.
- `F7` writes a field checkpoint to `quicksaves/quick.xgqs`.
- `F8` validates and loads that checkpoint, including its field, entrance, game
  state, and player position. Save/load requests made during dialogue,
  transitions, scripted control, or a menu are queued until the next safe field
  frame because this is a field checkpoint rather than a full process savestate.
- `F9` starts or stops an MP4 recording in `recordings/`. Linux recording
  requires `ffmpeg` and `pactl`; it records the completed game framebuffer as
  H.264 and the default PipeWire/Pulse output monitor as AAC stereo. Because the
  monitor is the system output, other desktop sounds are recorded too.

Set `XENO_QUICKSAVE_PATH`, `XENO_RECORDING_DIR`, or
`XENO_RECORDING_AUDIO_SOURCE` to override the checkpoint file, recording
directory, or Pulse monitor source respectively.

### Linux portability notes (handled in `CMakeLists.txt`, no vendored edits)
- PsyCross's bundled CMake is bypassed (case-sensitive globs miss its uppercase
  `*.C` PSX files; it expects SDL2/OpenAL as subprojects). We compile its sources
  directly and use system SDL2/OpenAL/OpenGL.
- `src/port_compat.h` is force-included to supply `strcasecmp`/`strncasecmp`
  (PsyCross's `include/psx/strings.h` shadows the system header).
- `-fpermissive` downgrades 64-bit function-pointer-to-int casts in the PSX SDK
  code (Phase 1 will revisit any that land on the boot path).

## Files
- `CMakeLists.txt` — native build: compiles PsyCross + the port executable.
- `src/port_main.c` — entry point; brings up PsyCross (Phase 1 hands off to the game).
- `src/xeno_pc.h` — port build identity + `INCLUDE_ASM` neutralisation plumbing.
- `src/port_compat.h` — Linux/gcc compat shim for PsyCross.
- `extern/PsyCross/` — vendored HAL (gitignored).
