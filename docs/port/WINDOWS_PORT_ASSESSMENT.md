# Windows port: assessment (parked)

Status: **parked** (owner, 2026-09-29: "just do Linux for now"). Read-only
assessment of commit 6fe9cb86; nothing was built. Revisit after r6.

## Verdict

Feasible via MinGW-w64 cross-compilation in docker; MSVC is not viable
(`pc_port/build_port.sh` depends on GNU ld/objcopy/nm behaviour and link-map
parsing). Medium effort: roughly 1-2 weeks to a booting Windows x64 build,
then an open-ended testing tail on real Windows GPU/audio drivers.

## Already portable

- PsyCross is Windows-first upstream (`_WIN32` branches, MinGW handling in
  `platform.h`); all of its threads, timers and audio go through SDL2 /
  OpenAL, both of which ship Windows builds.
- Port-side pthreads, `clock_gettime`, `opendir`, `stat`: provided by MinGW
  (winpthreads).
- No X11, no sockets, no `mmap` (the emulated RAM is a BSS array).

## Risks, ranked

1. **64-bit `long`.** Linux x86-64 is LP64, Windows is LLP64. About 980
   `u_long` and 1,470 `long` uses were debugged with 8-byte `long`, e.g. the
   `OT_TAG` pad in `patches/psycross_port_prelude.patch`. Every stride,
   offset and pad needs an audit.
2. **Weak symbols on COFF.** `objcopy --weaken-symbol` on game objects
   (`build_port.sh` override ownership and overlay clashes) and ~199 weak
   attributes; GNU ld on PE may resolve them differently.
3. **Address limits.** OT links are 24-bit, so `g_PsxRam` must sit below
   16 MiB: needs a low image base, `--disable-dynamicbase`,
   `--disable-high-entropy-va`, and a build-time check of its address.
4. **Battle runtime** uses `dlsym(RTLD_DEFAULT)` and `/proc/self/maps`
   (`battle_mips_runtime.c`): replace with an export table /
   `GetProcAddress` and `VirtualQuery`.
5. **Text-mode `open()`** without `O_BINARY` (`pc_file_io.c`, `krom_rom.c`):
   silent corruption on Windows.
6. **Dependencies/CI.** A new image with mingw-w64 and wine64, MinGW builds of
   SDL2, OpenAL and OpenSSL (or replace OpenSSL with an embedded SHA-1/256).
7. **Chores.** `HOME`/XDG to `%APPDATA%` (6 sites), `mkdir(path, mode)` (6),
   `readlink /proc/self/exe` (3), `realpath` (1), compile out the
   fork/ffmpeg recorder and `.so` plugins; the missing-file dialog in
   `disc_check.c` is gated on `DISPLAY`/`WAYLAND_DISPLAY`.

## Cheapest de-risking step

A 1-2 day compile-only spike in a MinGW docker layer: check how the
weakened overrides resolve and where `g_PsxRam` lands. That settles risks 2
and 3 and firms up the estimate. Interim option for Windows users: the Linux
build under WSL2/WSLg.
