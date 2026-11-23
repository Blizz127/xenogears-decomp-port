"""Validate the actual Windows executable's runtime battle data lookup table."""
import json
from pathlib import Path
import re
import struct


def linked_symbols(map_path, wanted):
    result = {}
    contribution = None
    for line in map_path.read_text().splitlines():
        match = re.fullmatch(r"([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+\d+ (\s*)(.+)", line)
        if not match:
            continue
        rva, length = (int(word, 16) for word in match.group(1, 2))
        indent, label = match.group(3, 4)
        if not indent:
            contribution = None
        elif origin := re.fullmatch(r"(.+):\(([^()]*)\)", label):
            contribution = (rva, length, Path(origin.group(1)).name)
        elif label in wanted and contribution:
            result.setdefault(label, set()).add((rva, *contribution))
    return result


def main_image_bridge_rows(bridge_map, runtime_source):
    base = re.search(r"^#define BATTLE_BASE\s+(0x[0-9a-fA-F]+)u\s*$",
                     runtime_source.read_text(), re.MULTILINE)
    if not base:
        raise ValueError("could not identify the runtime's battle overlay boundary")
    overlay = int(base.group(1), 16)
    pattern = re.compile(r'\s*\{ (0x[0-9a-fA-F]+)u, \d+u, ([01]), "(\w+)" \},\s*')
    result = []
    for line in bridge_map.read_text().splitlines():
        if not line.lstrip().startswith("{"):
            continue
        row = pattern.fullmatch(line)
        if not row:
            raise ValueError(f"invalid battle symbol map row: {line}")
        address, function, name = row.groups()
        if int(address, 16) < overlay:
            result.append((int(address, 16), function == "1", name))
    if not result:
        raise ValueError("empty main-image battle bridge map")
    return result


def verify(binary, exports, bridge_map, map_path, evidence_path):
    # exports comes from the actual PE export address table, including aliases.
    evidence = json.loads(evidence_path.read_text())
    rows = evidence["symbols"]
    runtime = Path(__file__).resolve().parents[2] / "src/battle_mips_runtime.c"
    mapped = {name for _, function, name in main_image_bridge_rows(bridge_map, runtime) if not function}
    explicit = set(evidence["required"])
    required = explicit | (mapped & exports.keys())
    missing = required - (rows.keys() & exports.keys())
    if missing:
        raise ValueError("unverified runtime PE data bindings: " + ", ".join(sorted(missing)))

    table = "PcPort_Win32DataSymbolSize.sizes"
    locations = linked_symbols(map_path, required | {table})
    pe = binary.read_bytes()
    header = struct.unpack_from("<I", pe, 60)[0]
    count = struct.unpack_from("<H", pe, header + 6)[0]
    optional = header + 24
    optional_size = struct.unpack_from("<H", pe, header + 20)[0]
    base = struct.unpack_from("<Q", pe, optional + 24)[0]
    sections = []
    for index in range(count):
        offset = optional + optional_size + index * 40
        virtual_size, start, raw_size, raw = struct.unpack_from("<4I", pe, offset + 8)
        flags = struct.unpack_from("<I", pe, offset + 36)[0]
        if flags & 0x40000000 and not flags & 0x20000000:  # readable data without execute
            sections.append((start, virtual_size, raw, raw_size))

    def contents(rva, count):
        for start, size, raw, raw_size in sections:
            if start <= rva and rva + count <= start + min(size, raw_size):
                offset = raw + rva - start
                return pe[offset:offset + count]
        raise ValueError(f"missing linked initializer at RVA {rva:#x}")

    # A fresh metadata file beside a stale executable must never pass. Read every
    # fixed-address table entry and its pointed-to C string back out of the PE.
    candidates = locations.get(table, set())
    if len(candidates) != 1:
        raise ValueError("cannot locate the compiled Windows binding table")
    table_rva, origin, length, owner = next(iter(candidates))
    if owner != "battle_win32_host.c.o" or not origin <= table_rva <= origin + length - len(rows) * 16:
        raise ValueError("compiled binding table not within its Windows bridge object")
    data = contents(table_rva, len(rows) * 16)
    for i, name in enumerate(sorted(rows)):
        pointer, size = struct.unpack_from("<QQ", data, 16 * i)
        expected = name.encode("ascii") + b"\0"
        if size != rows[name]["size"] or contents(pointer - base, len(expected)) != expected:
            raise ValueError(f"compiled PE table disagrees with target evidence at {name}")

    for name in sorted(required):
        rva = exports[name]
        size = rows[name]["size"]
        if size <= 0 or not any(start <= rva and rva + size <= start + length
                                for start, length, _, _ in sections):
            raise ValueError(f"battle binding {name} exceeds mapped PE data")
        for actual, origin, length, owner in locations.get(name, ()):
            if (actual == rva and rows[name]["origins"].get(owner) == rva - origin
                    and origin <= rva and rva + size <= origin + length):
                break
        else:
            raise ValueError(f"{name}: PE export does not match a proven COFF data object and extent")
    return f"Verified {len(required)} runtime data lookups and all {len(rows)} compiled PE size entries"
