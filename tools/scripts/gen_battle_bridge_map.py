#!/usr/bin/env python3
"""Generate the address/name table used by the retail battle MIPS adapter.

Addresses come from splat symbol_addrs files (the retail link map).  ELF input
is used only to classify symbol names as code or data; its current section
layout is deliberately not used as address authority.

Classification of the *retail library* range (heap/CD/SPU/GTE, 0x80019C7C ..
0x8004EA90) cannot come from the matching ELF: that code is not decompiled, so
it has no FUNC symbol there.  A retail `jal` into it is still a function call,
and the interpreter refuses to run one that the table calls data ("unresolved
native call target=0x80032498 HeapChangeCurrentUser" was one, 2026-09-18).
Two extra sources of evidence are therefore used: the port binary itself
(--host-elf: a name the port defines as a FUNC is bindable), and the PsyQ heap
family, which is entirely function code in this executable.

--size-from-host BINARY (the linked port binary; build_port.sh's second pass)
sizes the data bindings from the native objects the runtime binds them to
(its dlsym view, .dynsym): every host-bound data entry gets the explicit size
min(guest extent, host object size), and a host-defined D_XXXXXXXX object
that starts inside another host-bound entry's extent gets its own entry, so
guest accesses to it reach that native object instead of running past the
wider one (e.g. D_800594D5/6, the G/B bytes after D_800594D4).  Without the
flag the output is unchanged.
"""

import argparse
import os
import re
import subprocess


ASSIGN_RE = re.compile(
    r"^\s*([A-Za-z_.$][\w.$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;"
    r"(?:.*?\bsize:\s*(0x[0-9A-Fa-f]+|\d+))?"
)


def elf_function_names(paths):
    names = set()
    for path in paths:
        if not os.path.exists(path):
            continue
        output = subprocess.run(
            ["readelf", "-sW", path], check=True, text=True,
            capture_output=True
        ).stdout
        for line in output.splitlines():
            fields = line.split()
            if len(fields) >= 8 and fields[0].endswith(":"):
                if fields[3] == "FUNC" and fields[6] != "UND":
                    names.add(fields[7])
    return names


BATTLE_BASE = 0x8006FAF0          # battle_mips_runtime.c: overlay data stays guest
MAIN_EXE_DATA_START = 0x80010000
D_NAME_RE = re.compile(r"^D_([0-9A-Fa-f]{8})$")
# Guest ranges that must stay guest-resident even where the port defines a
# same-named object (native code reads or writes them in guest RAM directly):
# the retail overlays write D_8006F9DC.. with no native owner
# (battle_mips_runtime.c, g_GameState), and temp3.c reads the encounter
# record at 0x8006F9DE through PSX_ADDR.  No split entry is added there.
GUEST_RESIDENT = (
    (0x8006F9DC, BATTLE_BASE),
    # world_map_terminal_default_71264.c stores this byte to guest RAM directly.
    (0x800591AE, 0x800591AF),
)


def host_data_sizes(path):
    """name -> st_size of the defined data objects in BINARY's .dynsym."""
    output = subprocess.run(
        ["nm", "-D", "--defined-only", "-S", path], check=True, text=True,
        capture_output=True
    ).stdout
    sizes = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) == 4 and fields[2] in "BDRVGSC":
            sizes[fields[3]] = int(fields[1], 16)
    return sizes


def inferred_size(rows, index):
    """battle_mips_runtime.c inferred_data_size over these rows."""
    address, _, size, _ = rows[index]
    if size:
        return size
    for next_address, _, _, is_function in rows[index + 1:]:
        if is_function or next_address <= address:
            continue
        if next_address - address <= 0x10000:
            return next_address - address
        break
    return 4


def sort_rows(rows):
    return sorted(rows, key=lambda row: (row[0], not row[3], row[1]))


def size_from_host(rows, binary):
    host = host_data_sizes(binary)
    bound = [(address, inferred_size(rows, i))
             for i, (address, name, _, is_function) in enumerate(rows)
             if not is_function and address < BATTLE_BASE and name in host]
    starts = [address for address, _ in bound]
    known = {name for _, name, _, _ in rows}
    added = []
    for name in sorted(host):
        match = D_NAME_RE.match(name)
        if name in known or not match:
            continue
        address = int(match.group(1), 16)
        if not MAIN_EXE_DATA_START <= address < BATTLE_BASE:
            continue
        if any(lo <= address < hi for lo, hi in GUEST_RESIDENT):
            continue
        # The last host-bound entry starting before it: is it inside its extent?
        lo, hi = 0, len(starts)
        while lo < hi:
            mid = (lo + hi) // 2
            if starts[mid] < address:
                lo = mid + 1
            else:
                hi = mid
        if lo and address < bound[lo - 1][0] + bound[lo - 1][1]:
            added.append((address, name, 0, False))
    rows = sort_rows(list(rows) + added)
    sized = []
    for i, (address, name, size, is_function) in enumerate(rows):
        if not is_function and address < BATTLE_BASE and host.get(name):
            size = min(inferred_size(rows, i), host[name])
        sized.append((address, name, size, is_function))
    return sized


def parse_symbols(paths, functions, host_functions=()):
    by_key = {}
    for path in paths:
        with open(path, encoding="utf-8") as source:
            for line in source:
                match = ASSIGN_RE.match(line)
                if not match:
                    continue
                name = match.group(1)
                if name.startswith(".L") or name.startswith("L8"):
                    continue
                address = int(match.group(2), 0)
                size = int(match.group(3), 0) if match.group(3) else 0
                is_function = (
                    name in functions or name in host_functions or
                    name.startswith("func_") or
                    # The PsyQ heap library lives in the un-decompiled retail
                    # library range and is pure function code in this EXE.
                    name.startswith("Heap") or
                    name in {
                        "bzero", "memcpy", "memmove", "memset", "printf",
                        "rand", "ratan2", "rcos", "rsin",
                    }
                )
                key = (address, name)
                old = by_key.get(key)
                if old is None or size > old[0]:
                    by_key[key] = (size, is_function)
    return sort_rows(
        (address, name, size, is_function)
        for (address, name), (size, is_function) in by_key.items())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--elf", action="append", default=[])
    parser.add_argument("--symbols", action="append", required=True)
    parser.add_argument("--host-elf", action="append", default=[],
                        help="port binary/object(s): a name they define as FUNC "
                             "is evidence that retail code at that address is a "
                             "bindable function (un-decompiled library range)")
    parser.add_argument("--size-from-host",
                        help="linked port binary: size host-bound data entries to "
                             "min(guest extent, host object) and split extents at "
                             "host-defined D_ objects inside them")
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    functions = elf_function_names(args.elf)
    host_functions = elf_function_names(args.host_elf)
    symbols = parse_symbols(args.symbols, functions, host_functions)
    if args.size_from_host:
        symbols = size_from_host(symbols, args.size_from_host)
    with open(args.out, "w", encoding="utf-8") as out:
        out.write("/* Generated from retail symbol maps; do not edit. */\n")
        out.write("static const PcPortBattleSymbol g_BattleBridgeSymbols[] = {\n")
        for address, name, size, is_function in symbols:
            escaped = name.replace("\\", "\\\\").replace('"', '\\"')
            out.write(
                f'    {{ 0x{address:08x}u, {size}u, '
                f'{1 if is_function else 0}, "{escaped}" }},\n'
            )
        out.write("};\n")
        out.write(
            "static const size_t g_BattleBridgeSymbolCount = "
            "sizeof(g_BattleBridgeSymbols) / sizeof(g_BattleBridgeSymbols[0]);\n"
        )


if __name__ == "__main__":
    main()
