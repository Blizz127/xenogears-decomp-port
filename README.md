# xenogears-decomp-port

Xenogears Decomp and Port: the source of a native PC port layer for
Xenogears (PlayStation, USA, SLUS-00664), built on a matching decompilation
of the game.

> **You need your own disc.** This repository contains no game data, no BIOS
> and no Sony SDK code. The port reads everything from files you extract from
> your own copy of Xenogears. See [NOTICE.md](NOTICE.md).
>
> **Just want to play?** Jump to [How to play](#how-to-play).

## About this project

This is a passion project. I'm working hard on it, but it's made for fun and for everyone's enjoyment — free, non-commercial, and made by a fan. If you enjoy it, that's the whole point.

## How to play

A native Linux build of Xenogears, not an emulator. You need your own copy of the game; nothing from the game is included. No BIOS is needed.

### 1. Download

From the [Releases page](https://github.com/Blizz127/xenogears-decomp-port/releases), download **`xenogears-port-linux-x86_64-r5-e1625759.tar.gz`** (and `SHA256SUMS` if you want to check it).

### 2. Install

Unpack it anywhere. For example, into `~/Games`:

```sh
mkdir -p ~/Games
tar xzf ~/Downloads/xenogears-port-linux-x86_64-r5-e1625759.tar.gz -C ~/Games
```

That gives you `~/Games/xeno-r5-e1625759/`, with `xenogears.sh` inside.

### 3. Your disc

You need **Xenogears (USA) Disc 1** as a raw **BIN/CUE** image (the "Redump" kind: one track, 718,738,272 bytes). An `.iso` does not work.

- Ripped with a disc tool to BIN/CUE: use `disc1.bin` as it is.
- You have a `.chd`: convert it with `chdman extractcd -i "Xenogears (USA) (Disc 1).chd" -o disc1.cue -ob disc1.bin`. `chdman` comes with MAME; on Bazzite or Steam Deck, install `mame-tools` in a distrobox.

Put it in a folder of its own, for example `~/Games/xenogears/disc/disc1.bin`.

**This pre-release also needs six files extracted from the disc.** The next release will do this step for you. The extraction takes a minute, using the original decompilation's extractor:

```sh
cd ~/Games/xenogears
git clone --depth 1 https://github.com/ladysilverberg/xenogears-decomp
curl -L -o xenogears-decomp/tools/scripts/extract_exe.py \
  https://raw.githubusercontent.com/Blizz127/xenogears-decomp-port/main/tools/scripts/extract_exe.py
python3 -m venv venv && venv/bin/pip install pyyaml
venv/bin/python xenogears-decomp/tools/scripts/extract_overlays.py disc/disc1.bin --extract-to disc
venv/bin/python xenogears-decomp/tools/scripts/extract_exe.py disc/disc1.bin --output disc/SLUS_006.64
```

Afterwards `~/Games/xenogears/disc/` holds `disc1.bin`, `SLUS_006.64`, `field.bin`, `menu.bin`, `shop_menu.bin`, `member_change_menu.bin` and `world_map.bin`. Extra `.bin` files from the extractor are harmless. The port checks every file on start and tells you which one is missing or wrong.

### 4. Run it

```sh
~/Games/xeno-r5-e1625759/xenogears.sh ~/Games/xenogears/disc
```

The first time, give it your disc folder as shown. It remembers the folder, so afterwards `xenogears.sh` alone is enough, or you can double-click it in a file manager. It starts fullscreen; `XENOGEARS_WINDOWED=1 ~/Games/xeno-r5-e1625759/xenogears.sh` opens a window instead. The first start checks the disc image, which takes a few seconds.

### 5. Steam Deck / Bazzite Game Mode

1. In Desktop Mode, run it once from a terminal with your disc folder (step 4), so the folder is remembered.
2. In Steam, choose **Games → Add a Non-Steam Game → Browse**, and pick `~/Games/xeno-r5-e1625759/xenogears.sh`.
3. Switch to Game Mode and start it from your library. No launch options are needed.
4. Controller: the built-in controls work as a standard gamepad. Use Steam's default "Gamepad" layout.

### Controls

| Action | Gamepad (Xbox/Deck labels) | Keyboard |
|---|---|---|
| Move | D-pad or left stick | Arrow keys |
| Confirm / talk | A | Z |
| Menu | X | V |
| Other face buttons | B, Y | C, X |
| Rotate camera | L1 / R1 (LB / RB) | Left Shift / Right Shift |
| L2 / R2 | LT / RT | Left Ctrl / Right Ctrl |
| Start / Select | Menu / View | Enter / Space |

Port extras (keyboard, or click the toolbar at the top of the screen):
- F7: quick save; F8: quick load (in the field).
- F9: record video.
- F11: game speed (Shift+F11 resets it). Hold Backspace for 5× fast-forward.
- F10: random battles on or off. F12: "god mode".

### Troubleshooting

- **Nothing happens, or a launcher says "runtime error".** The port didn't find your files. Run `xenogears.sh ~/Games/xenogears/disc` from a terminal and read the message.
- **"retail data check failed: missing retail file …".** One of the six files is missing or came from a different release. Redo step 3. The message says which file and what it should be.
- **"disc image check failed".** The image isn't the USA Disc 1 raw BIN (a wrong region, an `.iso`, or a modified image).
- **`GLIBC_2.34' not found`.** Your Linux is too old. The build needs glibc 2.34 or newer (2021 or later distributions, including SteamOS 3 and Bazzite).
- **The game closes when I choose Attack in a battle.** A known bug in this pre-release, affecting normal on-foot battles (the opening Gear battle is fine). A fix is in progress. Quick-save with F7 in the field before battles.
- **Movies flicker in Game Mode.** A known issue, being fixed. Playing in Desktop Mode or in a window (`XENOGEARS_WINDOWED=1`) may help. Press Cross/A to skip a movie.
- **Logs** are written to `~/.local/share/xenogears-port/Xenogears (PC port).log`. More detail appears when you run it from a terminal. Saves and memory cards are in the same folder.

### FAQ

- **Do I need a PlayStation BIOS?** No. The port includes a public-domain font instead.
- **Is the game included?** No. There are no game files, music or movies. You must own the disc, and the port reads your copy.
- **How far does it go?** Title screen, New Game, the prologue and Lahan, including the first Gear battle. The route toward Black Moon Forest stops at the Mountain Path (map 15) for now.

## What is here

- `pc_port/`: the port. It runs the game's decompiled C natively on Linux
  (x86-64), with the PlayStation hardware provided by
  [PsyCross](https://github.com/OpenDriver2/PsyCross) and SDL2. It includes
  the host layer (window, input, audio, saves), overrides and dispatch
  tables, the disc and retail-file checks, the launcher packaging, mods and
  cheat hooks, and about 1,300 tests.
- `tools/`: analysis tools, including the retail-data and Psy-Q guards that
  keep game and SDK bytes out of the tree, the `remu` MIPS reference
  emulator, and helper scripts.
- `config/`: splat and symbol configuration for the overlays mapped after the
  fork (addresses and names only).
- `docs/`: the port architecture, the retail-divergence log, the audit, the
  modding guide, [FAKEMATCHES.md](docs/FAKEMATCHES.md), and wiki pages.

## What is not here, and why

The decompiled game code (`src/`, `include/`) and the matching-build files
are **not** included. The decompilation this work continues,
[ladysilverberg/xenogears-decomp](https://github.com/ladysilverberg/xenogears-decomp),
has not published a license, so its code, and files built on it, are not
redistributed here. Also left out: Sony Psy-Q headers and decompiled SDK
libraries, the compiler binaries, prebuilt third-party tools, a test sprite
asset, and every file that matched retail bytes.

## Building

The port compiles together with the decompiled game code, so it cannot be
built from this repository alone. You need:

1. the Xenogears decompilation source tree
   ([ladysilverberg/xenogears-decomp](https://github.com/ladysilverberg/xenogears-decomp)
   and its setup instructions), with this repository's `pc_port/`, `tools/`
   and `config/` added on top;
2. your own disc: the Disc 1 image and the files the decomp's extraction
   scripts write from it (`SLUS_006.64`, `field.bin`, `menu.bin`,
   `shop_menu.bin`, `member_change_menu.bin`, `world_map.bin`) in `disc/`;
3. the build container described in `pc_port/README.md`, then
   `pc_port/build_port.sh`.

The port currently tracks decompiled functions that are newer than the
upstream tree, so building against upstream as it is today is expected to
leave some functions missing. This repository publishes the port source for
reference, review and preservation. Game data and binaries are never
distributed.

## Status

Boot, title screen, New Game, the opening, and Lahan play on the port. The
Black Moon Forest route is in progress. See
[docs/port/RETAIL_DIVERGENCES.md](docs/port/RETAIL_DIVERGENCES.md) for every
known difference from the PlayStation original.

## Credits and license

- Credits: [CREDITS.md](CREDITS.md). The upstream decomp authors, the font
  authors, and the tools and libraries.
- License: [LICENSE](LICENSE) (MIT) for the original work in this repository
  only.
- Legal notice and disclaimer: [NOTICE.md](NOTICE.md). Xenogears is © Square
  Enix. This project is not affiliated with or endorsed by Square Enix.
