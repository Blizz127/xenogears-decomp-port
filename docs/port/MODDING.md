# Modding the Xenogears PC port

The port loads mods from folders on your computer. A mod can replace game
assets (files, textures, sound samples), run C code on game events, or both.
Cheats are built in and need no mod.

The port reads everything from **your own disc**. Mods never ship game data:
a replacement is something you or a modder made, and the originals you dump
to edit stay on your machine. Mod folders are ignored by git; never commit
them to this repository.

Architecture background: [ARCHITECTURE.md](ARCHITECTURE.md).

## Where mods live

The port scans, in this order:

1. `mods/` in the working directory, or the folder set by `mods.dir` in
   `config.ini` (`--set mods.dir=/path` / `XENO_MODS_DIR`);
2. `${XDG_DATA_HOME:-~/.local/share}/xenogears-port/mods/`.

Each mod is one sub-folder:

```
mods/
  my_mod/
    mod.txt                 manifest (required)
    assets/<sha256>.bin     asset replacements (optional)
    plugin.so               C plugin (optional)
```

Turn all mods off with `mods.enabled = off` in `config.ini`, or
`XENO_MODS_ENABLED=0`. With no mods installed nothing changes: the game runs
exactly as without the mod layer.

At start-up the port prints one line per mod loaded:

```
[xg_plat] mod 0: Hello Plugin 1.0 (load order 50) from mods/hello_plugin
[xg_plat] mods: 1 loaded, 0 asset replacement(s)
```

## mod.txt

`key = value` lines; `#` starts a comment.

| Key | Meaning |
|---|---|
| `name` | display name (required) |
| `version` | free text |
| `load_order` | integer, default 0. Lower loads first; a later mod overrides an earlier one's replacement of the same asset. Ties load by name. |
| `abi` | mod ABI version the mod was made for. Must be `1` (the current `XG_PLAT_MODS_ABI_VERSION`); other values are skipped with a message. |
| `script` | reserved for Lua scripts (see [Lua](#lua-reserved)); not run yet. |

## Replacing assets

Replacements are keyed by the SHA-256 of the **original bytes** the game
read. So a replacement never depends on file names or disc offsets, and it
cannot apply to the wrong data.

What can be replaced:

| Kind | What is hashed | Size rule |
|---|---|---|
| archive file | the whole file as the loader read it | must fit the game's buffer |
| streamed file (field sound banks) | the whole streamed file | any size; served sector by sector |
| texture | the pixel block of one `LoadImage` upload (16-bit VRAM data) | exactly the same size (`w*h*2` bytes) |
| sound samples | one `SpuWrite` block (PSX ADPCM) | the same size or smaller (zero-padded) |

A replacement that breaks a size rule is refused with a message on stderr,
and the original is used.

The streamed-file path is covered live by `pc_port/tests/run_stream_replacement_live_test.sh`.

### Finding and dumping the originals

1. See what the game loads:

   ```
   XENO_MODS_LOG_ASSETS=1 ./xeno-port
   ```

   prints the hash of every file, texture and sample block as it is loaded.

2. Save the originals from **your disc** to edit them:

   ```
   XENO_MODS_DUMP_DIR=dump ./xeno-port
   ```

   writes each one as `dump/<sha256>.bin`. Play to the scene you want to
   change so its assets get loaded.

3. Edit a file, then save it under its **original** name in your mod:
   `mods/my_mod/assets/<sha256-of-the-original>.bin`.

Texture blocks are raw 16-bit PSX VRAM (5:5:5 + mask bit, or palette indices
for 4/8-bit textures with their CLUT uploaded separately). Sound blocks are
PSX SPU ADPCM.

Keep dumps private: they are copies of game data.

## C plugins

A mod may contain `plugin.so`, a shared library that exports:

```c
#include "xg_plat/mods.h"
int xg_mod_init(const XgPlatModApi* api);   /* return 0 on success */
```

`xg_mod_init` runs once at start-up. It receives:

| `XgPlatModApi` field | Use |
|---|---|
| `abi` | the port's mod ABI version; refuse to load (return non-zero) if it is not the one you built against |
| `subscribe(event, hook, user)` | call `hook(event_data, user)` on each event; returns a handle |
| `unsubscribe(handle)` | stop a subscription |
| `log(fmt, ...)` | print a `[mod]` line on stderr |
| `reserved[]` | reserved for later ABI 1 additions (Lua bridge); always NULL now |

Events (`XgPlatModEventData`: `event`, `arg0`, `arg1`, `sequence`):

| Event | When | `arg0` |
|---|---|---|
| `XG_PLAT_EVENT_BOOT` | once, before the game's main loop | |
| `XG_PLAT_EVENT_FRAME_TICK` | after every presented frame, on the game thread | |
| `XG_PLAT_EVENT_ASSET_LOADED` | a game file was read | size in bytes |
| `XG_PLAT_EVENT_ROOM_ENTER` | the player entered a field map | field map number |

Hooks run on the game thread; keep them short. The built-in cheats are
themselves `frame_tick` subscribers.

### Example

`pc_port/examples/mods/hello_plugin/` is a complete code-only mod that logs
each field map entered. Build and install it from the repository root:

```
mkdir -p mods/hello_plugin
cc -shared -fPIC -Ipc_port/include -o mods/hello_plugin/plugin.so \
   pc_port/examples/mods/hello_plugin/plugin.c
cp pc_port/examples/mods/hello_plugin/mod.txt mods/hello_plugin/
```

Run the port from the folder that contains `mods/`; you see
`[mod] hello_plugin: loaded` at start-up and a line per map change.

## Cheat console

Cheats are named commands that run on the game thread at the next frame.
They come from the host toolbar and hotkeys (F10 / F12), or from standard
input when `cheats.console = on` is set in `config.ini`
(`--set cheats.console=on`).

| Command | Effect |
|---|---|
| `help` | list the commands |
| `god [on\|off]` | invincibility (toggles without an argument) |
| `encounters [on\|off]` | random battles on/off |
| `save` / `load` | quick checkpoint save / restore |
| `warp <map> <x> <z>` | go to a field map and position |
| `battle <id>` | start battle `<id>` |
| `speed <1..5>` | game speed multiplier |
| `bind` / `bind <button> <key>` | list the key bindings / rebind a pad button to an SDL key name (for example `bind cross Space`); permanent bindings go in `config.ini` as `input.<button> = <key>` |
| `mods` | list the loaded mods |

## Lua (reserved)

Lua scripting is planned but not implemented. The slots are reserved now so
mods written today keep working when it arrives:

- `mod.txt` key `script = <file>.lua`: recognised and reported
  ("reserved, not run"); the mod's assets and plugin still load.
- `XgPlatModApi.reserved[]` (ABI 1): later additions, including the script
  bridge, fill these; plugins must not use them.
- Scripts will receive the same events as plugins (`boot`, `frame_tick`,
  `asset_loaded`, `room_enter`) and the cheat-console commands.

A change that breaks existing plugins raises `XG_PLAT_MODS_ABI_VERSION`;
mods that declare the old `abi` are then skipped with a message instead of
misbehaving.
