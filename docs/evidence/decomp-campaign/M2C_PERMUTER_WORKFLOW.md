# m2c / permuter workflow

The helper scripts live in `tools/scripts/decomp_assist/`. They read only the
splat output (`asm/`) and the local build. Scratch files go to
`$XENO_ASSIST_CACHE` (default `~/.cache/xeno/decomp_assist`). Builds run on the
host unless `XENO_BUILD_WRAP` names a wrapper, for example a docker runner.
m2c (`XENO_M2C`, default `~/.cache/xeno/m2c/m2c.py`) and decomp-permuter are
external clones and are not vendored.

`make rom-check` is the authority. A function counts as matched only when the
overlay rebuilds byte-identically and rom-check stays 10/10. Every script below
is a way to get there faster, and none of them replaces rom-check.

## Scripts

| Script | What it does |
| --- | --- |
| `m2c_try.py OV SEG [names]` | Drafts each INCLUDE_ASM function with m2c (`-t mips-gcc-c --valid-syntax`), compiles it standalone with the repo's cc1 + maspsx, and compares it with the retail words (relocations masked). Writes `results.json`. |
| `m2c_insert.py OV SEG names` | Replaces those functions' `INCLUDE_ASM(...)` lines in `src/OV/SEG.c` with the cleaned drafts. It merges the drafts' externs and prototypes and reconciles conflicting extern types. |
| `m2c_drive.sh OV VRAM SEG...` | Runs the whole loop: m2c_try, insert every standalone match, `make build`. It skips functions that fail to compile, or that `elf_cmp.py` shows mismatching inside their TU, and repeats until `build/out/OV.bin` equals `disc/OV.bin`. |
| `elf_cmp.py OV VRAM` | Masked per-function compare of the linked overlay against retail. |
| `try_func.py OV NAME [--keep]` | Flips one `#ifndef XENO_PC_PORT INCLUDE_ASM / #else <C>` function to C, rebuilds only its object, and shows an aligned diff. It reverts on a mismatch. |
| `alt_cc.py OV` | First pass for stubborn regalloc/`mfhi` near-misses: recompiles the cached drafts with each tools/ cc1 (2.6.3, 2.6.0, 2.7.2-cdk) and lists what matches under which. A hit means the TU may belong under that compiler's gears.toml preset, if the whole TU stays byte-exact (bcf1 is 2.6.3 code; five battle TUs are too). Drafts for such a TU are compiled with `XENO_CC1=tools/<cc>/cc1`. |
| `perm_setup.py OV SEG NAME [BASE.c]` | Prepares a decomp-permuter directory (base.c, target.o, compile.sh). Run it as `nice -n 15 permuter.py -j1 --best-only --stop-on-zero DIR`, one at a time and never alongside a rom-check. |

A standalone match is not proof. The same draft can compile differently inside
its TU when an earlier declaration there gives a global or callee another type,
so m2c_drive always rebuilds and compares the whole overlay.

## Lessons that turned near-misses into matches

- **Void callees.** m2c guesses `int` for an unknown callee. If the call's result
  is never used, that guess keeps `v0` live across the call and changes register
  allocation. Declaring the callee `void` fixed 11 bcf1 handlers at once; the
  permuter found this on `func_battle_command_file1_801E6144`. m2c_try does it
  automatically (`AUTO_VOID=1`).
- **Array-typed externs.** gcc-2.7.2 keeps a global's address in a register
  (`lui/addiu`, then `lw/sw 0(reg)`), and schedules a store to it before later
  loads, only when the extern has array type. Declare `extern T X[];` and access
  `X[0]` (field script-VM setters, `D_800B2348`, `D_80076F3C`). For a struct
  global accessed by raw offset, use a typed alias:
  `extern s16 g_Scene_s16[] asm("g_Scene");`.
  `m2c_try.py` drafts this form with `ARRAYIFY=all` (or named symbols). When
  the TU already declares the global as a scalar, `m2c_insert.py` adds an
  `asm("sym")` alias for the array view, as it did for battling's
  `func_80080090` family.
- **Reload after aliasing stores.** Retail re-reads a global pointer such as
  `g_FieldScriptVMCurActor` after any store that could alias it, and indexes
  `scripts[curScriptIndex]` inline. Caching either in a local changes the code.
- **Unused leading parameters.** m2c drops them, but retail still reads its
  operands from `a1`/`a2`. m2c_try restores them.
- **Signedness and casts.** An explicit `(u16)` on a value passed to an
  unprototyped callee, a `u32` comparison where retail uses `slt`, or `lh` vs
  `lhu` is often the whole difference.
- **INCLUDE_ASM order.** INCLUDE_ASM must keep source order with the C bodies.
  The maspsx dummy-function wrapper in `include/include_asm.h` does this. A bare
  top-level `__asm__` is emitted ahead of every C function at -O2.
- **Divide checks.** A retail signed `div` followed by `break 7` / `break 6`
  checks was assembled with maspsx `--expand-div`. Give such a TU a preset
  with that flag, as `BattleDiv` / `BattleGcc263Div` do for main68 / main69.
  `alt_cc.py` tries it automatically.
- **objdiff is not byte-exact.** It scored field's `func_800A7948` 100% while two
  stores went to swapped `symbol + offset` targets.
- **Epilogue delay filling.** Our gcc-2.7.2 cc1 can move a function's last
  body instruction after `lw $ra` (into the load's delay) and put
  `addiu $sp` in the `jr` slot. Retail menu never does this: 0 of 253 retail
  epilogues. When a mismatch is exactly "last insn swapped with `lw $ra`",
  and that insn is a store, make the store `volatile`
  (`*(volatile u8*)&p->field = v;`). Volatile insns are not delay-slot
  candidates. That matched menu's func_801D28FC, func_801E649C and
  func_801E64E0. When the last insn is a constant return (`return 1;`, e.g.
  func_801DE29C) there is no known fix yet. `-fno-delayed-branch` is not the
  answer: menu matched 145 functions with it, against 204 without.
- **Load order after stores.** When retail re-reads a field right after
  storing to a neighbouring one, where gcc could prove the addresses differ
  and so keeps the value in a register, reading it through
  `*(volatile T*)` reproduces the reload (menu func_801E42AC, func_801E4998,
  func_801E35BC).
- **Operand order of `addu`.** gcc canonicalises pointer + int to
  `addu base, off`. When retail has `addu off, base`, compute the offset into
  an `s32`/`uintptr_t` temp and add `(uintptr_t)base`. `uintptr_t` is 32-bit
  in the retail headers and host-wide on the port, so the same expression is
  safe in shared bodies (menu func_801D0FD4, func_801D9E3C, func_801E4258).
- **Menu-style shared TUs.** Where a file keeps `#ifndef XENO_PC_PORT
  INCLUDE_ASM / #else <port body>` sites, run `m2c_insert.py --inner` so the
  draft becomes the retail arm of the existing definition rather than a
  second definition. Menu compiles with `-G0` (gears.toml MenuG0), so set
  `XENO_GP_FLAG=-G0` for m2c_try/m2c_insert there.
- **Survey other cc1 builds when a mismatch is systematic.** If the same
  codegen difference shows up across many unrelated functions and no source
  shape moves it (menu: every magic-multiply division wrote `mfhi` into a
  scratch register where retail writes the result register), the TU was
  probably built with a different compiler. The decomp.me ps1 compilers are
  Linux cc1 builds from
  `https://github.com/decompals/old-gcc/releases` (the Dockerfiles under
  `platforms/ps1/` in `decompme/compilers` give the exact tarball per
  version). Download just the cc1 of each candidate (a few MB each) into
  `~/.cache/xeno/compilers/`, preprocess the TU once, and compile that `.i`
  with each cc1 using the TU's flags. Then count per-function matches against
  retail twice: once on the current source, and once on an early, all-C
  version of the TU (so already-landed shapes tuned for one compiler don't
  bias the vote). Pick a build that is a superset on the all-C source and
  fixes the systematic difference; for menu that was gcc-2.6.3-psx (205 vs
  204, plus seven parked functions matching outright). Then add the cc1 under
  `tools/gcc-X-psx/` and select it with a per-TU preset (gears.toml
  `MenuG0`), recording the URL and cc1 sha256 in the preset comment. Re-fit
  the few bodies that were tuned for the old compiler, and gate on a full
  rom-check.
