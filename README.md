# xenogears-decomp-port

Xenogears Decomp and Port: the source of a native PC port layer for
Xenogears (PlayStation, USA, SLUS-00664), built on a matching decompilation
of the game.

> **You need your own disc.** This repository contains no game data, no BIOS
> and no Sony SDK code. The port reads everything from files you extract from
> your own copy of Xenogears. See [NOTICE.md](NOTICE.md).

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
