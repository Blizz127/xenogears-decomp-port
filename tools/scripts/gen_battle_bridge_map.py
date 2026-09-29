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
    return sorted(
        ((address, name, size, is_function)
         for (address, name), (size, is_function) in by_key.items()),
        key=lambda row: (row[0], not row[3], row[1]),
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--elf", action="append", default=[])
    parser.add_argument("--symbols", action="append", required=True)
    parser.add_argument("--host-elf", action="append", default=[],
                        help="port binary/object(s): a name they define as FUNC "
                             "is evidence that retail code at that address is a "
                             "bindable function (un-decompiled library range)")
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    functions = elf_function_names(args.elf)
    host_functions = elf_function_names(args.host_elf)
    symbols = parse_symbols(args.symbols, functions, host_functions)
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
