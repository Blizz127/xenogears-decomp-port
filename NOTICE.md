# Notice

## Ownership

Xenogears, and all of its code, data, artwork, music, text and trademarks, are
owned by Square Enix. This project is not affiliated with, endorsed by or
sponsored by Square Enix or Sony Interactive Entertainment. "PlayStation" is a
trademark of Sony Interactive Entertainment.

## What this repository contains

Only original work: the source of a native PC port layer and its host code,
analysis and build tools, scripts, tests, configuration and documentation.
The MIT license in [LICENSE](LICENSE) applies to that original work only.

This repository does **not** contain:

- any game content: no disc images, no files extracted from the disc, no
  audio, textures, movies, text or other assets;
- any BIOS image or bytes from one;
- any Sony or Psy-Q SDK code, headers, libraries or compiler binaries;
- the decompiled game code of the upstream decompilation project (see
  [README.md](README.md#building) and [CREDITS.md](CREDITS.md)).

Every file was checked before publishing with the project's retail-data
guard (`tools/analysis/retail_data_guard.py --strict`, against the retail
files and the BIOS image) and the Psy-Q guard
(`tools/analysis/psyq_port_guard.py`). The only matches the strict guard
reports are the guard's documented "address-table" class: sequences of
function addresses, the same facts a symbol map records, in
`pc_port/src/world_map_scheduler.c` and `pc_port/src/data_slus_sdata.c`.
They are not game data.

## Your own disc

To run anything built from this code you need your own legally obtained copy
of Xenogears (USA, SLUS-00664). The port reads all game data from files you
supply from that disc at runtime. Nothing here lets you play the game without
it.

## Purpose

The decompilation and port are made for preservation, research and
interoperability: keeping the game playable on current hardware for people
who own it.

## Third-party components

Third-party components keep their own licenses; see
[CREDITS.md](CREDITS.md) and [pc_port/THIRD_PARTY.md](pc_port/THIRD_PARTY.md).
