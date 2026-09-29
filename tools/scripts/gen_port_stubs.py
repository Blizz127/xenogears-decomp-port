#!/usr/bin/env python3
"""
Generate placeholder stubs for the native PC port (Phase 1).

In the port build, INCLUDE_ASM is neutralised, so every not-yet-decompiled
function becomes an undefined symbol at link time. This tool takes the list of
undefined symbols (e.g. from `nm -u` over the compiled game objects), classifies
each as a function or data object (with size) using the matching build's ELF
symbol tables, and emits a C file that:

  - defines each missing FUNCTION as a stub that logs the first time it is hit
    (the "oracle": it tells you exactly which function the boot path needs next)
    and returns 0;
  - defines each missing DATA symbol as zeroed storage of the right size.

Usage:
    nm -u game_objs/*.o | awk '{print $2}' | sort -u > undef.txt
    python3 tools/scripts/gen_port_stubs.py \
        --elf build/out/slus_006.64.elf --elf build/out/field.elf \
        --undefined undef.txt --out pc_port/generated/stubs.c
"""

import argparse
import os
import re
import subprocess
import sys


# Matches a splat symbol_addrs line carrying a size annotation, e.g.
#   g_GameState = 0x8006D634; // type:GameState size:0x2300
# The ELF symbol table frequently records size 0 for data globals (they live in
# .bss/COMMON), so these annotations are the only reliable struct sizes -- without
# them a struct like g_GameState gets a 16-byte stub and field code reading e.g.
# partyMembers at offset 0x1D34 walks off the end into other stubs.
_SIZE_RE = re.compile(
    r"^\s*([A-Za-z_.$][\w.$]*)\s*=\s*0x[0-9A-Fa-f]+\s*;.*?\bsize:\s*(0x[0-9A-Fa-f]+|\d+)"
)


def parse_symbol_sizes(paths):
    """Return {name: size} from symbol_addrs files' `size:` annotations (max wins)."""
    sizes = {}
    for p in paths:
        try:
            with open(p) as fh:
                lines = fh.readlines()
        except FileNotFoundError:
            print(f"  (warning: symbol-addrs file not found: {p})", file=sys.stderr)
            continue
        for line in lines:
            m = _SIZE_RE.match(line)
            if not m:
                continue
            try:
                sz = int(m.group(2), 0)
            except ValueError:
                continue
            name = m.group(1)
            if sz > sizes.get(name, 0):
                sizes[name] = sz
    return sizes


# Matches a splat symbol_addrs type annotation, e.g.
#   firstfile = 0x80040584; // type:func
# splat writes `type:func` for names it knows are code even when an overlay ELF
# only carries them as ABS (undefined) references. That is exactly the case for
# firstfile/nextfile: they live in the main executable's .text, but the menu
# overlay links them as ABS symbols, so ELF/map classification alone sees a
# NOTYPE symbol with no section and would emit a zeroed data array.
_TYPE_RE = re.compile(
    r"^\s*([A-Za-z_.$][\w.$]*)\s*=\s*0x[0-9A-Fa-f]+\s*;.*?\btype:\s*([A-Za-z_]\w*)"
)


def parse_symbol_types(paths):
    """Return {name: type} from symbol_addrs `type:` annotations (last wins)."""
    types = {}
    for p in paths:
        try:
            with open(p) as fh:
                lines = fh.readlines()
        except FileNotFoundError:
            print(f"  (warning: symbol-addrs file not found: {p})", file=sys.stderr)
            continue
        for line in lines:
            m = _TYPE_RE.match(line)
            if m:
                types[m.group(1)] = m.group(2).lower()
    return types


def classify_symbols(elf_paths):
    """Return {name: (is_func, size)}.

    Real functions are typed FUNC by splat/ld; data globals are emitted as
    NOTYPE (or OBJECT). So: FUNC -> function, any other defined symbol -> data.
    """
    table = {}
    for elf in elf_paths:
        out = subprocess.run(
            ["readelf", "-sW", elf], capture_output=True, text=True, check=True
        ).stdout
        for line in out.splitlines():
            parts = line.split()
            # Num:  Value  Size  Type  Bind  Vis  Ndx  Name
            if len(parts) < 8 or not parts[0].endswith(":"):
                continue
            size_s, kind, ndx, name = parts[2], parts[3], parts[6], parts[7]
            if ndx == "UND" or not name:
                continue
            try:
                size = int(size_s, 0)
            except ValueError:
                size = 0
            is_func = (kind == "FUNC")
            prev = table.get(name)
            # Prefer a concrete (non-ABS) sighting; a FUNC typing wins; keep max size.
            if prev is None:
                table[name] = (is_func, size, ndx != "ABS")
            else:
                p_func, p_size, p_concrete = prev
                table[name] = (
                    p_func or is_func,
                    max(p_size, size),
                    p_concrete or (ndx != "ABS"),
                )
    return {k: (v[0], v[1]) for k, v in table.items()}


_MAP_SECTION_RE = re.compile(r"^ \.(\w+)\s+0x[0-9A-Fa-f]+")
_MAP_SYMBOL_RE = re.compile(r"^\s+(0x[0-9A-Fa-f]+)\s+(\S+)\s*$")


def classify_from_maps(map_paths):
    """Return {name: (is_func, size)} from GNU ld --Map files.

    Symbols listed under a `.text` output section are functions; everything
    else (`.data`, `.sdata`, `.bss`, `.sbss`, `.rodata`, COMMON) is data.
    Size is left at 0 so symbol_addrs annotations and the 16-byte floor in
    main() still own layout. ELF classification, when present, wins.
    """
    table = {}
    for path in map_paths:
        try:
            with open(path) as fh:
                lines = fh.readlines()
        except FileNotFoundError:
            print(f"  (warning: map file not found: {path})", file=sys.stderr)
            continue
        in_text = False
        for line in lines:
            sec = _MAP_SECTION_RE.match(line)
            if sec:
                in_text = sec.group(1) == "text"
                continue
            if "COMMON" in line[:16]:
                in_text = False
                continue
            m = _MAP_SYMBOL_RE.match(line)
            if not m:
                continue
            name = m.group(2)
            if not name or name.startswith(".") or name == "*fill*":
                continue
            prev = table.get(name)
            if prev is None:
                table[name] = (in_text, 0)
            else:
                table[name] = (prev[0] or in_text, prev[1])
    return table


def classify_by_name_prefix(name):
    """Last-resort type for splat-style names the ELF/map missed.

    `func_*` is always a function in this tree. `D_*` / `g_*` / jump tables
    are data. Returns None when the name is not in those families so the
    generator still refuses to guess arbitrary identifiers.
    """
    if name.startswith("func_"):
        return (True, 0)
    if name.startswith(("D_", "g_", "jtbl_", "jpt_")):
        return (False, 0)
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", action="append", default=[],
                        help="matching-build ELF(s) used to classify symbols")
    parser.add_argument("--map", action="append", default=[],
                        help="GNU ld --Map file(s); used when an overlay ELF is "
                             "missing (e.g. slus_006.64.map)")
    parser.add_argument("--undefined", default="-",
                        help="file with undefined symbol names (default: stdin)")
    parser.add_argument("--symbol-addrs", action="append", default=[],
                        help="splat symbol_addrs file(s); their size: annotations "
                             "give data globals their real struct sizes")
    parser.add_argument("--out", required=True, help="output C file path")
    args = parser.parse_args()

    if not args.elf and not args.map:
        print("ERROR: provide at least one --elf or --map for symbol classification.",
              file=sys.stderr)
        return 1

    if args.undefined == "-":
        names = sys.stdin.read().split()
    else:
        with open(args.undefined) as fh:
            names = fh.read().split()
    # These helpers are emitted by the generator itself, not retail symbols.
    helper_names = {"xeno_port_stub", "xeno_port_is_generated_stub"}
    names = sorted(set(n for n in names if n) - helper_names)

    table = classify_symbols(args.elf)
    for name, info in classify_from_maps(args.map).items():
        if name not in table:
            table[name] = info
    prefixed = 0
    unknown = []
    for n in names:
        if n in table:
            continue
        guessed = classify_by_name_prefix(n)
        if guessed is None:
            unknown.append(n)
            continue
        table[n] = guessed
        prefixed += 1
    if unknown:
        print("ERROR: matching ELFs/maps do not classify all undefined symbols; "
              "refusing to guess function/data types or overwrite stubs.", file=sys.stderr)
        print("  Unclassified: " + ", ".join(unknown), file=sys.stderr)
        print("  Build the missing matching overlays and retry.", file=sys.stderr)
        return 1
    if prefixed:
        print(f"  classified {prefixed} leftover splat-style names by prefix "
              f"(func_/D_/g_/jtbl_/jpt_)", file=sys.stderr)
    sym_sizes = parse_symbol_sizes(args.symbol_addrs)
    # `type:func` overrides an ABS/NOTYPE (data-looking) ELF sighting. This is
    # the only signal for a .text function that overlay ELFs reference as ABS.
    sym_types = parse_symbol_types(args.symbol_addrs)

    funcs, data, resized = [], [], 0
    for n in names:
        info = table.get(n)
        if info[0] or n.startswith("func_") or sym_types.get(n) == "func":  # is_func
            # Some overlay entrypoints are referenced from the main executable at
            # addresses that are BSS in the main map. Prefer the code-style name
            # over NOTYPE/BSS so native calls cannot bind to data storage.
            funcs.append((n, info[1]))
        else:
            # NOTYPE/OBJECT data. Base size = largest of the ELF size, the
            # symbol_addrs `size:` (the only reliable struct size), and a 16-byte
            # floor. Then DOUBLE it: all these sizes describe the 32-bit PSX layout
            # (4-byte pointers), but the 64-bit port widens every pointer to 8 bytes,
            # so a void*[3] (12 bytes on PSX) needs 24, and a pointer-heavy struct
            # up to 2x. Doubling is the safe upper bound (pointer-only data is exactly
            # 2x; byte/int data merely over-reserves zeroed memory). Without this,
            # e.g. g_PartyDataBuffers[2] reads off the end of its stub.
            base = max(info[1], sym_sizes.get(n, 0), 16)
            if sym_sizes.get(n, 0) > max(info[1], 16):
                resized += 1
            data.append((n, base * 2))

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w") as f:
        f.write("/* AUTO-GENERATED by tools/scripts/gen_port_stubs.py - do not edit. */\n")
        f.write("/* Placeholder stubs for not-yet-decompiled symbols (PC port Phase 1). */\n\n")
        f.write("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n")
        f.write("/* Logs the first time each stub is reached: the boot-path oracle. */\n")
        f.write("void xeno_port_stub(const char* name) {\n")
        f.write("    static const char* seen[4096];\n")
        f.write("    static int n = 0;\n")
        f.write("    for (int i = 0; i < n; i++) if (seen[i] == name) return;\n")
        f.write("    if (n < 4096) seen[n++] = name;\n")
        f.write("    fprintf(stderr, \"[stub] %s\\n\", name);\n")
        f.write("}\n\n")

        f.write(f"/* ---- {len(data)} data symbols ---- */\n")
        for name, size in data:
            f.write(f"unsigned char {name}[{size}] __attribute__((aligned(8)));\n")

        # The helper above is itself an undefined ref during the trial link
        # (game_overrides calls it before stubs.o exists). Never emit a stub
        # for it — that would conflict with the real void(const char*) definition.

        f.write(f"\n/* ---- {len(funcs)} function symbols ---- */\n")
        for name, _size in funcs:
            f.write(f"long {name}(void) {{ xeno_port_stub(\"{name}\"); return 0; }}\n")

        f.write("\n/* Fail-closed query used by retail-code adapters. */\n")
        f.write("int xeno_port_is_generated_stub(const char* name) {\n")
        f.write("    static const char* const names[] = {\n")
        for name, _size in funcs:
            f.write(f"        \"{name}\",\n")
        f.write("    };\n")
        f.write("    size_t i;\n")
        f.write("    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)\n")
        f.write("        if (strcmp(name, names[i]) == 0) return 1;\n")
        f.write("    return 0;\n")
        f.write("}\n")

    print(f"Wrote {args.out}: {len(funcs)} function stubs, {len(data)} data symbols "
          f"({resized} sized from symbol_addrs)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
