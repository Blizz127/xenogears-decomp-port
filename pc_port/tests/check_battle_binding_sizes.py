#!/usr/bin/env python3
"""Oversized battle data bindings in a linked port binary.

The battle runtime binds a main-executable global's guest range to native
storage (rt.data, battle_mips_runtime.c initialize_runtime): every bridge-map
data symbol below BATTLE_BASE whose name dlsym finds, sized by
inferred_data_size (the map's explicit size, else the distance to the next
data symbol, at most 0x10000, else 4; g_GameState at least 0x2358), plus the
explicit tables of initialize_model_node_data / initialize_model_packet_data.
A binding wider than the ELF object it lands on lets a guest access past the
object's end reach its link neighbours.  This mirrors that table from the
binary's dynamic symbols (the port links -rdynamic) and the generated
battle_bridge_map.inc; the runtime logs the same check at battle start
(runtime_binding_size_guard, "[xeno-port][battle-bindings]").

usage: check_battle_binding_sizes.py XENO_PORT BRIDGE_MAP_INC RUNTIME_C
           [--allow FILE] [--list]
Exit 1 when an oversized binding is not in the allow file (one
"<guest address> <binding size> <object size> <name>" line each, sizes in
hex; '#' comments), or when an allowed entry no longer matches (fixed or
changed: update the file).
"""
import argparse
import re
import subprocess
import sys

BATTLE_BASE = 0x8006FAF0


def dynamic_symbols(binary):
    """name -> (address, size) for defined dynamic data/bss/rodata symbols."""
    out = subprocess.run(["nm", "-D", "--defined-only", "-S", binary],
                         check=True, capture_output=True, text=True).stdout
    symbols = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 4:
            address, size, kind, name = parts
            symbols[name] = (int(address, 16), int(size, 16), kind)
        elif len(parts) == 3:   # no size
            address, kind, name = parts
            symbols[name] = (int(address, 16), 0, kind)
    return symbols


def bridge_symbols(path):
    entry = re.compile(r'\{\s*0x([0-9a-fA-F]+)u,\s*(\d+)u,\s*([01]),\s*"([^"]+)"\s*\}')
    with open(path) as f:
        return [(int(a, 16), int(s), int(fn), name)
                for a, s, fn, name in entry.findall(f.read())]


def inferred_size(table, index):
    address, size, _, _ = table[index]
    if size:
        return size
    for next_address, _, is_function, _ in table[index + 1:]:
        if is_function or next_address <= address:
            continue
        if next_address - address <= 0x10000:
            return next_address - address
        break
    return 4


def function_body(source, name):
    start = source.index("static void %s(BattleMipsRuntime *runtime)" % name)
    return source[start:source.index("\n}\n", start)]


def bindings(binary, bridge_map, runtime_c):
    dyn = dynamic_symbols(binary)
    table = bridge_symbols(bridge_map)
    data = {}
    for i, (address, _, is_function, name) in enumerate(table):
        if address >= BATTLE_BASE or is_function or name not in dyn:
            continue
        size = inferred_size(table, i)
        if name == "g_GameState" and size < 0x2358:
            size = 0x2358
        data[address] = (size, name)
    with open(runtime_c) as f:
        source = f.read()
    for address, name in re.findall(r'\{0x([0-9a-fA-F]+)u,"([^"]+)"\}',
                                    function_body(source, "initialize_model_node_data")):
        if name in dyn:
            data[int(address, 16)] = (4, name)   # appended: a second slot
    for address, size, name in re.findall(r'\{0x([0-9a-fA-F]+)u,(\d+)u,(NULL|"[^"]+")\}',
                                          function_body(source, "initialize_model_packet_data")):
        address = int(address, 16)
        if name == "NULL":
            data.pop(address, None)              # guest RAM: no host object
        elif name.strip('"') in dyn:
            data[address] = (int(size), name.strip('"'))
    return data, dyn


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("binary")
    ap.add_argument("bridge_map")
    ap.add_argument("runtime_c")
    ap.add_argument("--allow")
    ap.add_argument("--list", action="store_true")
    args = ap.parse_args()
    data, dyn = bindings(args.binary, args.bridge_map, args.runtime_c)
    oversized = {}
    for address, (size, name) in sorted(data.items()):
        _, object_size, _ = dyn[name]
        if object_size and size > object_size:
            oversized[address] = (size, object_size, name)
    allowed = {}
    if args.allow:
        with open(args.allow) as f:
            for line in f:
                line = line.split("#", 1)[0].split()
                if line:
                    allowed[int(line[0], 16)] = (int(line[1], 16), int(line[2], 16), line[3])
    status = 0
    for address, (size, object_size, name) in sorted(oversized.items()):
        known = allowed.get(address) == (size, object_size, name)
        if args.list or not known:
            print("%s %08x %x %x %s" % ("allowed" if known else "NEW", address, size,
                                        object_size, name))
        if not known:
            status = 1
    for address, (size, object_size, name) in sorted(allowed.items()):
        if oversized.get(address) != (size, object_size, name):
            print("STALE %08x %x %x %s (no longer oversized as listed: update the allow file)"
                  % (address, size, object_size, name))
            status = 1
    print("battle bindings: %d host-bound, %d oversized, %d allowed, status %s"
          % (len(data), len(oversized), len(allowed), "FAIL" if status else "ok"))
    return status


if __name__ == "__main__":
    sys.exit(main())
