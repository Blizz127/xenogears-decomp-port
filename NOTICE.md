# Notice

## Ownership

Xenogears, and all of its code, data, artwork, music, text and trademarks, are
owned by Square Enix. This project is not affiliated with, endorsed by or
sponsored by Square Enix or Sony Interactive Entertainment. "PlayStation" is a
trademark of Sony Interactive Entertainment.

## What this repository contains

- The source of a matching decompilation of Xenogears (USA, SLUS-00664): C
  source written to compile back to the same machine code as the retail
  game, with its headers, splat configuration, symbol maps and build files.
- The native PC port layer and host code built on it, plus analysis and build
  tools, scripts, tests and documentation.

The MIT license in [LICENSE](LICENSE) applies only to the original work in this
repository.

### Upstream-derived files

This repository includes files derived from the upstream decompilation
[ladysilverberg/xenogears-decomp](https://github.com/ladysilverberg/xenogears-decomp).
That project has not published a license. Its contributors' work is included
with credit, remains theirs, and is **not** covered by this repository's MIT
license. Every such file is listed, with its upstream authors, in
[UPSTREAM_FILES.md](UPSTREAM_FILES.md). The owner is re-deriving those parts
independently, and each file leaves the list once its upstream-authored parts
are replaced.

## Supplying game and development files

Players supply their own US Xenogears disc. The original PlayStation font also requires the player's own BIOS. The matching build uses a separately supplied toolchain. The PC build uses the project interface declarations in `include/psyq/` and the renderer described below.

## Historical checks and public releases

The legacy default branch and r5 release predate the current source and executable audit. Earlier reports classified several arrays of consecutive addresses as symbol information. The forward changes remove the copied tables and test fixtures or obtain them from the player's disc. The two symbol catalogs retain individual address assignments. Matching MIPS tools may emit jump tables while compiling code from a local split of the player's copy.

A review of v0.7.93-preview found original battle and palette dispatch records and gear-model fields in the Windows and Linux executables. The legacy PC movie player also transcribed parts of Sony's VLC decoder and CD stream ring. The early SDK and retail byte scans missed these findings. Each later release records the exact archive digests, source revision, platform test results and scan coverage in its own notes.

### PsyCross

The port uses [PsyCross](https://github.com/OpenDriver2/PsyCross) (MIT) for the
PlayStation hardware and API layer. It is not part of this repository.
`pc_port/build_port.sh` fetches it at build time at a pinned commit and
patches it with `pc_port/patches/`. The upstream checkout contains GTE lookup tables. The port patch replaces their inclusion with storage initialized from the player's own executable. The square-root routine reads the player's data through guest memory.

## Your own disc

To build or run anything from this code you need your own legally obtained
copy of Xenogears (USA, SLUS-00664). The build takes the parts of the game it
does not have as source from files you extract from that disc, and the port
reads all game data from them at runtime. Nothing here lets you play the game
without it.

## Purpose

The decompilation and port are made for preservation, research and
interoperability: keeping the game playable on current hardware for people
who own it.

## Third-party components

Third-party components keep their own licenses; see
[CREDITS.md](CREDITS.md) and [pc_port/THIRD_PARTY.md](pc_port/THIRD_PARTY.md).
