# xenogears-decomp-port

Xenogears Decomp and Port: the source of a native PC port layer for
Xenogears (PlayStation, USA, SLUS-00664), built on a matching decompilation
of the game.

> **You need your own disc.** The port reads game assets from your own copy of Xenogears.
> See the ownership and release history in [NOTICE.md](NOTICE.md).
>
> **Just want to play?** Jump to [How to play](#how-to-play).

## About this project

This is a passion project. I'm working hard on it, but it's made for fun and for everyone's enjoyment — free, non-commercial, and made by a fan. If you enjoy it, that's the whole point.

## How to play

The follow-up audit found original battle and palette dispatch records and gear-model fields in both v0.7.93-preview executables. Its movie player also included transcriptions from Sony’s video decoder and CD stream library. Earlier checks missed these items. That release remains available as a historical preview. The forward source skips pre-rendered cinematics, including the New Game intro, while an independent movie implementation is pending.

For a subsequent build, check its [release notes](https://github.com/Blizz127/xenogears-decomp-port/releases) for the short-record scan result. Download the Windows x64 ZIP or Linux x86_64 tar.gz and its `SHA256SUMS`. Extract the complete platform archive. Its README lists the supported disc images, file hashes, and license notices.

Use Python 3 and your own US Disc 1 as a raw, single-track MODE2/2352 BIN. From the extracted package, substitute your image path in the commands below. The preparation tool verifies the image and prepares the game data locally.

**Windows PowerShell**

```powershell
py -3 .\prepare-data\prepare.py "D:\My Discs\Disc 1.bin" "$env:LOCALAPPDATA\Xenogears Data"
py -3 .\start-windows.py "$env:LOCALAPPDATA\Xenogears Data"
```

**Linux**

```sh
python3 prepare-data/prepare.py "/path/to/Disc 1.bin" "$HOME/Xenogears Data"
./xenogears.sh "$HOME/Xenogears Data"
```

At the title screen, select **New Game** and press **Z** to confirm. The preview runs compiled host game code and retains a PlayStation CPU interpreter for uncovered paths. The v0.7.93-preview release was tested through Fei's first free movement in Lahan on Linux and under Wine. See the release notes for verified gameplay, known issues, and physical-device coverage. The port includes a public-domain font. To use the original font, set `XENO_BIOS` to your own SCPH-5500 BIOS dump.

## What is here

- `src/`, `include/`: the matching decompilation of the game, as C source and
  headers. Some of it is upstream work, included with credit; see
  [UPSTREAM_FILES.md](UPSTREAM_FILES.md).
- `config/`, `Makefile`, `gears.toml`, `Dockerfile`: the splat
  configuration, symbol maps (addresses and names only) and build files.
- `pc_port/`: the port. It runs the game's decompiled C natively on Linux
  (x86-64), with the PlayStation hardware provided by
  [PsyCross](https://github.com/OpenDriver2/PsyCross) and SDL2. It includes
  the host layer (window, input, audio, saves), overrides and dispatch
  tables, the disc and retail-file checks, the launcher packaging, mods and
  cheat hooks, and about 1,300 tests.
- `tools/`: the build driver (`gears`), analysis tools including the
  retail-data and Psy-Q guards that keep game and SDK bytes out of the tree,
  the `remu` MIPS reference emulator, and helper scripts.
- `docs/`: the port architecture, the retail-divergence log, the audit, the
  modding guide, [FAKEMATCHES.md](docs/FAKEMATCHES.md), and wiki pages.

Not here: game data, the BIOS, the Sony Psy-Q SDK, compiler and other
prebuilt binaries, and a few game source files that still hold retail data
(see [What does not build yet](#what-does-not-build-yet)).

## Building from source

This section is for people who want to build the decompilation or the port
themselves. To play, the [release build](#how-to-play) is all you need.

### What you need

- **Linux on x86-64**, with either **Docker** (recommended) or these
  packages: `git make curl python3 python3-venv binutils-mips-linux-gnu
  cpp-mips-linux-gnu` (Ubuntu 24.04 names), and the Python packages in
  `requirements.txt` (`python3 -m venv .venv && .venv/bin/pip install -r
  requirements.txt`). The included `Dockerfile` sets all of this up.
- **Rust** (`cargo`, from [rustup.rs](https://rustup.rs)) to build the build
  driver, `tools/gears`. Without it the fetch script falls back to
  upstream's older prebuilt copy.
- **Your own disc**: Xenogears (USA) Disc 1 as a raw BIN, as in
  [How to play](#how-to-play), placed at `disc/disc1.bin`.

### 1. Get the source and the pieces not redistributed here

```sh
git clone --recursive https://github.com/Blizz127/xenogears-decomp-port
cd xenogears-decomp-port
tools/fetch_upstream_toolchain.sh
```

The repository supplies the project-authored declarations used by the code in `include/psyq/`. The toolchain setup preserves those headers.

`tools/fetch_upstream_toolchain.sh` downloads what this repository does not
redistribute, into your own working tree (all of it is git-ignored):

- the GCC 2.6.0, 2.7.2 and 2.7.2-CDK PSX compilers, from the upstream
  decompilation ([ladysilverberg/xenogears-decomp](https://github.com/ladysilverberg/xenogears-decomp),
  pinned commit), and GCC 2.6.3 from [decompals/old-gcc](https://github.com/decompals/old-gcc),
  each checked by sha256;
- the decompiled Psy-Q libraries (`src/slus_006.64/psyq/`), from upstream.
  Upstream's copy is older than this project's, but SLUS_006.64 still builds
  byte-identical with it;
- `objdiff`, the `maspsx` submodule, and `gears`, built from `tools/gears/src`.

It never downloads game data.

### 2. Extract your disc files

```sh
python3 tools/scripts/extract_overlays.py disc/disc1.bin --extract-to disc
python3 tools/scripts/extract_exe.py disc/disc1.bin --output disc/SLUS_006.64
python3 tools/scripts/extract_battle_command_file1.py
python3 tools/scripts/extract_battle_module_0c4.py
```

(Use `.venv/bin/python` if you made a venv, or run them in the container as
in step 3, after `. /.venv/bin/activate`.) The files go in `disc/`, which
git ignores.

### 3. Build the matching decompilation

```sh
make                              # splat, compile, link into build/out/
ninja -k 0                        # finish the other targets after menu.elf fails (see below)
sha256sum --check config/checksum.sha
```

`make rom-check` is the full from-clean gate used in development. It cannot
pass until `menu.bin` builds (see below).

With Docker, run steps 2 and 3 in a container built from the included
`Dockerfile`. The build expects the tree at `/xenogears-decomp`:

```sh
docker build -t xenogears-build .
docker run --rm -it -u "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD":/xenogears-decomp xenogears-build \
  bash -c '. /.venv/bin/activate && make; ninja -k 0; sha256sum --check config/checksum.sha'
```

Run step 1 (the fetch script) on the host: it needs `git`, `curl` and, for
`gears`, `cargo`.

### 4. Build the PC port

```sh
pc_port/build_port.sh
```

This needs the matching build first, plus `cmake`, a C/C++ compiler, and
the development packages for SDL2, OpenAL, OpenGL and OpenSSL. See
[pc_port/README.md](pc_port/README.md). PsyCross is fetched at a pinned
commit and patched.

### What does not build yet

Nine game source files are **not yet publishable**. They still contain
retail bytes (inline `.word` bodies) or transcribed retail data tables, and
no public source has them:

| File | Why it is held back |
|---|---|
| `src/member_change_menu/main/misc.c` | inline retail bytes |
| `src/movie/main.c` | inline retail bytes |
| `src/slus_006.64/main/main_loop.c` | inline retail bytes |
| `src/slus_006.64/system/kernel_menu.c` | inline retail bytes |
| `src/slus_006.64/system/sound.c` | inline retail bytes and data tables |
| `src/slus_006.64/system/memory.c` | retail data tables and strings (found by a compiled-object scan) |
| `src/slus_006.64/system/animation_scripts.c` | a retail data table (found by a compiled-object scan) |
| `src/menu/main/misc.c` | retail strings of 32 bytes or more (found by a compiled-object scan) |
| `src/menu/main/misc3.c` | retail strings of 32 bytes or more (found by a compiled-object scan) |

What that means today. This was tested from a fresh clone of this repository,
with the fetch script, a USA Disc 1, and a container built from the included
`Dockerfile`:

- **Matching build (checked on the 23250136 snapshot; see Status for the current state): 9 of 10 targets build and match retail.** For each missing
  file, splat writes an assembly stub from your own disc, and the fetch script
  switches `memory.c`'s small data segment to assembly as well. `make` gets
  through splat and every compile step. `SLUS_006.64`, `field.bin`,
  `battle.bin`, `battling.bin`, `world_map.bin`, `movie.bin`,
  `shop_menu.bin`, `member_change_menu.bin` and `battle_command_file1.bin`
  link and match their `config/checksum.sha` hashes. **`menu.bin` does not
  link**, because `misc2.c` builds on the real `src/menu/main/misc.c`.
- **PC port: not buildable yet.** The port compiles these files as native C,
  where assembly stubs cannot stand in for them, so it needs all nine. It
  was not attempted for this snapshot.

These files will be published as their retail data moves to being loaded
from your disc at runtime.

## Status

This repository is a **source snapshot** of the private development tree (private
commit `d64e54d2`, after the earlier snapshot of `23250136`). It adds the
decompilation progress since then and the current port source. No new binary
release goes with it: **v0.5.0-r5 remains the only public binary.**

- The matching build of `field.bin` currently builds but does **not** match its
  pinned hash (known regression, being worked on); `menu.bin` still does not
  link without the held `misc.c`/`misc3.c`. The other targets match retail.
- The port's runtime is **hybrid**: unconverted battle code still runs through a
  retail MIPS interpreter that reads your own disc. A native-only build (no
  interpreter) is experimental and not playable yet.
- The PC port source in this tree is **known not to build as published**: a few port files are held back
  (the battle files that use a GTE macro header whose provenance is still under review), so the ported
  sources are out of step with the matching sources. `pc_port/build_port.sh` was not run for this snapshot.

Boot, title screen, New Game, the opening, and Lahan play on the r5 port. The
Black Moon Forest route is in progress. See
[docs/port/RETAIL_DIVERGENCES.md](docs/port/RETAIL_DIVERGENCES.md) for every
known difference from the PlayStation original.

## Credits and license

- Credits: [CREDITS.md](CREDITS.md). The upstream decomp authors, the font
  authors, and the tools and libraries.
- License: [LICENSE](LICENSE) (MIT) for the original work in this repository
  only. Files that include upstream work are listed in
  [UPSTREAM_FILES.md](UPSTREAM_FILES.md). They are not under the MIT license.
- Legal notice and disclaimer: [NOTICE.md](NOTICE.md). Xenogears is © Square
  Enix. This project is not affiliated with or endorsed by Square Enix.
