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

## What this repository does not contain

- Any game content: no disc images, no files extracted from the disc, no
  audio, textures, movies, text or other assets.
- Any BIOS image, or bytes from one.
- Any Sony or Psy-Q SDK code: no SDK headers (`include/psyq/`), no decompiled
  SDK libraries (`src/**/psyq/`), no SDK libraries or compiler binaries.
  `tools/fetch_upstream_toolchain.sh` downloads these from their public
  locations into your own working tree when you build. They are not
  redistributed here.
- Prebuilt third-party binaries (the build driver, objdiff) and the `maspsx`
  assembler wrapper (a git submodule).
- Game source files that still contain retail bytes or transcribed retail
  data tables. They are listed in [README.md](README.md#building-from-source)
  and stay out until that data is loaded from the user's disc instead.

## How the tree was checked

Before publishing, every file was checked:

1. **Retail-data guard, strict mode, against the retail files and the BIOS**
   (`tools/analysis/retail_data_guard.py --strict`, with SLUS_006.64, the nine
   overlay files and the SCPH-5500 BIOS as the reference). It found no
   "data"-class run: no game data or BIOS bytes. The only runs it reports are
   its "address-table" class, listed below.
2. **Compiled-object scan.** Every game translation unit included here was
   compiled, in both the matching (MIPS) and the port (x86-64) build, and the
   `.data`, `.rodata` and `.sdata` sections of each object were compared
   byte by byte against the same references. Translation units that produced
   32 bytes or more of retail data were left out.
3. **Psy-Q guard** (`tools/analysis/psyq_port_guard.py`): no Sony SDK code.
4. **No binaries** (`file` on every file) and no credentials or private
   hosts.

### Address tables

These runs are sequences of function or pointer addresses (or NULLs) that
also appear in the retail files as jump or pointer tables. They record the
same facts as the symbol maps in `config/` (where a function or variable
lives), not game data.

| File | Lines | Bytes | Retail location |
|---|---|--:|---|
| `config/symbol_addrs.field.txt` | 11–18 | 32 | field.bin, vaddr 0x800AE8E0 |
| `config/symbol_addrs.slus_006.64.txt` | 307–315 | 36 | SLUS_006.64, vaddr 0x8005072C |
| `pc_port/src/data_slus_sdata.c` | 122 | 64 | SLUS_006.64, vaddr 0x80050150 |
| `pc_port/src/world_map_scheduler.c` | 1365–1425 | 64 | world_map.bin, vaddr 0x80099EC4 |
| `pc_port/tests/sprite_dispatch_arithmetic_retail_test.c` | 95 | 40 | SLUS_006.64, vaddr 0x80018504 |
| `pc_port/tests/w34b3_prod_test.c` | 98–129 | 296 | world_map.bin, vaddr 0x80099F0C |
| `tools/remu/tests/diff_80085618.c` | 180–182 | 40 | battle.bin, vaddr 0x80070250 |
| `tools/remu/tests/diff_80085B58.c` | 226–228 | 40 | battle.bin, vaddr 0x80070250 |
| `tools/remu/tests/diff_80085C88.c` | 163–165 | 40 | battle.bin, vaddr 0x80070250 |

The compiled-object scan found one more address table: the matching build of
`src/field/main/misc4.c` emits a 128-byte jump table (`.rodata`) equal to the
one at field.bin vaddr 0x8006FB8C.

### PsyCross

The port uses [PsyCross](https://github.com/OpenDriver2/PsyCross) (MIT) for the
PlayStation hardware and API layer. It is not part of this repository.
`pc_port/build_port.sh` fetches it at build time at a pinned commit and
patches it with `pc_port/patches/`. Note that PsyCross's own source carries
GTE lookup tables that are byte-identical to the ones in Sony's libgte.

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
