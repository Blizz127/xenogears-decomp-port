#!/usr/bin/env python3
"""Reject port data that compact GPU links cannot represent.

The non-PIE port stores host addresses directly in the 24-bit links of GPU
packets (USE_EXTENDED_PRIM_POINTERS=0), so a packet that game code links
into an ordering table must lie below 0x1000000. Two checks:

1. g_FieldRenderContexts and g_PsxRam (native field packets, and the native
   HeapAlloc font packets in guest RAM) lie inside the compact range.
2. Every writable static data input (the .data and .bss output sections)
   of a project object -- game TUs, port sources, generated stubs -- ends
   below it, read from the link map. Game code links static packets kept in
   such objects: the generated DR_MODE stub D_800AFE24 once crossed the line
   and the field OT walk faulted at 0xf27. Host-only arrays tagged
   HOST_ONLY_BSS go to .lbss, which the linker places after .bss; they are
   listed, not checked. PsyCross and system library data is emulator and
   host state, not game packet storage (it already sat above 16 MB in the
   r6.19 release build); it is not checked.

It does not certify runtime packet pointers outside these objects.
"""
import argparse
import re
import subprocess

# 0 and 0xffffff have special link meanings. No byte may reach the
# terminator or wrap through the mask.
LIMIT = 0xffffff
CHECKED = (".data", ".bss")
HOST_ONLY = (".lbss", ".ldata")


def parse_map(path):
    """Return (output sections {name: (start, size)}, input rows) from a GNU ld
    map. An input row is (output section, input section, start, size, origin)."""
    outputs, rows = {}, []
    in_map, out_sec, wrapped = False, None, None
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if not in_map:
                in_map = line.startswith("Linker script and memory map")
                continue
            parts = line.split()
            if wrapped:
                kind, name = wrapped
                wrapped = None
                if len(parts) >= 2 and parts[0].startswith("0x") and parts[1].startswith("0x"):
                    if kind == "out":
                        outputs[name] = (int(parts[0], 16), int(parts[1], 16))
                    elif len(parts) >= 3:
                        rows.append((out_sec, name, int(parts[0], 16),
                                     int(parts[1], 16), " ".join(parts[2:])))
                    continue
            if not parts:
                continue
            if not line[0].isspace():
                # Column 0: an output section header, or LOAD/OUTPUT/etc.
                out_sec = parts[0] if parts[0].startswith(".") else None
                if out_sec and len(parts) == 1:
                    wrapped = ("out", out_sec)
                elif out_sec and len(parts) >= 3 and parts[1].startswith("0x"):
                    outputs[out_sec] = (int(parts[1], 16), int(parts[2], 16))
                continue
            if line.startswith("  ") or parts[0].startswith("*"):
                continue  # symbol/assignment lines, input patterns, *fill*
            if len(parts) == 1:
                wrapped = ("in", parts[0])
            elif len(parts) >= 4 and parts[1].startswith("0x") and parts[2].startswith("0x"):
                rows.append((out_sec, parts[0], int(parts[1], 16),
                             int(parts[2], 16), " ".join(parts[3:])))
    return outputs, rows


def project_object(origin):
    return not origin.startswith("/") and "libpsycross.a(" not in origin


def section_headers(binary):
    headers = {}
    for line in subprocess.check_output(["readelf", "-SW", binary], text=True).splitlines():
        m = re.search(r"\]\s+(\S+)\s+\S+\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s", line)
        if m:
            headers[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    return headers


def check_symbols(binary):
    lines = subprocess.check_output(
        ["nm", "-S", "--defined-only", binary], text=True
    ).splitlines()
    for symbol in ("g_FieldRenderContexts", "g_PsxRam"):
        rows = [line.split() for line in lines
                if line.split()[-1:] == [symbol]]
        if len(rows) != 1 or len(rows[0]) != 4:
            raise SystemExit(f"FAIL: missing or ambiguous {symbol} range")
        address, size, kind, _ = rows[0]
        start, length = int(address, 16), int(size, 16)
        end = start + length
        if kind not in ("B", "D") or not length or not 0 < start < end <= LIMIT:
            raise SystemExit(
                f"FAIL: {symbol} [{start:#x}, {end:#x}) exceeds compact GPU links"
            )
        print(f"PASS: compact {symbol} [{start:#x}, {end:#x})")


def check_map(binary, map_path):
    outputs, rows = parse_map(map_path)
    headers = section_headers(binary)
    for name in CHECKED:
        if name not in outputs or outputs[name] != headers.get(name):
            raise SystemExit(f"FAIL: {map_path} {name} {outputs.get(name)} does not "
                             f"match {binary} {headers.get(name)}; stale map?")
    checked = [r for r in rows if r[0] in CHECKED and r[3] and project_object(r[4])]
    if not checked:
        raise SystemExit(f"FAIL: no project data inputs parsed from {map_path}")
    bad = [r for r in checked if not 0 < r[2] < r[2] + r[3] <= LIMIT]
    for out_sec, name, start, size, origin in bad:
        print(f"FAIL: {origin} {name} [{start:#x}, {start + size:#x}) exceeds compact GPU links")
    if bad:
        raise SystemExit("FAIL: project data must end below 0x1000000; tag host-only "
                         "arrays HOST_ONLY_BSS (pc_port/include_shim/host_only_bss.h)")
    top = max(checked, key=lambda r: r[2] + r[3])
    end = top[2] + top[3]
    print(f"PASS: {len(checked)} project data inputs end below the compact line; highest "
          f"{top[4]} {top[1]} ends {end:#x}, headroom {LIMIT - end:#x} ({(LIMIT - end) >> 10} KB)")
    for out_sec, name, start, size, origin in rows:
        if out_sec in HOST_ONLY and size and project_object(origin):
            print(f"  host-only {out_sec}: {origin} [{start:#x}, {start + size:#x})")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("binary")
    parser.add_argument("--map", required=True, help="the binary's GNU ld link map")
    args = parser.parse_args()
    check_symbols(args.binary)
    check_map(args.binary, args.map)


if __name__ == "__main__":
    main()
