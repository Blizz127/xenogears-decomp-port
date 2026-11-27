#!/usr/bin/env python3
"""Read back target data extents and selected battle callbacks from a linked Windows image."""
import argparse
import json
import re
import shutil
import struct
import subprocess
from pathlib import Path


SIZE_TABLE = "PcPort_Win32DataSymbolSize.sizes"


def tool(name):
    path = shutil.which(name + "-21") or shutil.which(name)
    if not path:
        raise ValueError(f"{name} is required")
    return path


def read_exports(binary):
    output = subprocess.check_output([tool("llvm-readobj"), "--coff-exports", binary], text=True)
    rows = re.findall(r"^  Name: (.+)\n  RVA: (0x[0-9a-fA-F]+)$", output, re.M)
    result = {name: int(rva, 16) for name, rva in rows}
    if not rows or len(rows) != len(result):
        raise ValueError("empty or ambiguous PE export address table")
    return result


def linked_symbols(link_map, wanted, *, full_owner=False):
    """Associate each symbol RVA with its input object, contribution RVA and size."""
    result = {}
    contribution = None
    for line in link_map.read_text().splitlines():
        match = re.fullmatch(r"([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+\d+ (\s*)(.+)", line)
        if not match:
            continue
        rva, length = (int(word, 16) for word in match.group(1, 2))
        indent, label = match.group(3, 4)
        if not indent:
            contribution = None
        elif origin := re.fullmatch(r"(.+):\(([^()]*)\)", label):
            contribution = (rva, length, origin[1] if full_owner else Path(origin[1]).name)
        elif label in wanted and contribution:
            result.setdefault(label, set()).add((rva, *contribution))
    return result


class Image:
    def __init__(self, path):
        self.image = path.read_bytes()
        header = struct.unpack_from("<I", self.image, 60)[0]
        machine, count = struct.unpack_from("<HH", self.image, header + 4)
        optional = header + 24
        if (self.image[:2] != b"MZ" or self.image[header:header + 4] != b"PE\0\0" or
                machine != 0x8664 or struct.unpack_from("<H", self.image, optional)[0] != 0x20b):
            raise ValueError("expected an AMD64 PE32+ image")
        self.base = struct.unpack_from("<Q", self.image, optional + 24)[0]
        table = optional + struct.unpack_from("<H", self.image, header + 20)[0]
        self.code, self.data = [], []
        for offset in range(table, table + count * 40, 40):
            size, rva, raw_size, raw = struct.unpack_from("<4I", self.image, offset + 8)
            flags = struct.unpack_from("<I", self.image, offset + 36)[0]
            if flags & 0x20000000:
                self.code.append((rva, rva + size))
            elif flags & 0x40000000:
                self.data.append((rva, size, raw_size, raw))

    def contents(self, rva, length):
        for start, size, raw_size, raw in self.data:
            if start <= rva and rva + length <= start + min(size, raw_size):
                offset = raw + rva - start
                return self.image[offset:offset + length]
        raise ValueError(f"missing initialized PE data at RVA {rva:#x} length {length}")

    def executable(self, rva):
        return any(start <= rva < end for start, end in self.code)


def callback_owners(required_callbacks, exports, locations, image):
    if not isinstance(required_callbacks, dict) or not required_callbacks:
        raise ValueError("expected a nonempty selected callback mapping")
    names_by_rva = {}
    for name, rva in exports.items():
        names_by_rva.setdefault(rva, set()).add(name)
    callbacks = {}
    for name, owner in sorted(required_callbacks.items()):
        if (not isinstance(name, str) or not re.fullmatch(r"[a-zA-Z_][a-zA-Z0-9_]*", name)
                or not isinstance(owner, str) or not re.fullmatch(r"[a-zA-Z0-9_.-]+[.]o", owner)):
            raise ValueError("invalid callback name or COFF object basename in selection")
        rva = exports.get(name)
        if rva is None or not image.executable(rva):
            raise ValueError(f"selected callback lacks an executable PE export: {name}")
        if names_by_rva[rva] != {name}:
            raise ValueError(f"selected callback shares its exported address: {name}")
        found = locations.get(name, set())
        if len(found) != 1:
            raise ValueError(f"selected callback lacks a unique physical map owner: {name}")
        actual, start, length, physical = next(iter(found))
        if actual != rva or not start <= rva < start + length or Path(physical).name != owner:
            raise ValueError(f"selected callback export does not match {owner}: {name}")
        callbacks[name] = {"rva": hex(rva), "owner": owner}
    return callbacks


def verify(pe, link_map, bridge_map, evidence_path, root, linux_elf=None, *, required_callbacks):
    evidence = json.loads(evidence_path.read_text())
    sizes = evidence["symbols"]
    exports = read_exports(pe)
    image = Image(pe)
    runtime = (root / "pc_port/src/battle_mips_runtime.c").read_text()
    base = re.search(r"^#define BATTLE_BASE\s+(0x[0-9a-fA-F]+)u\s*$", runtime, re.M)
    if not base:
        raise ValueError("battle overlay boundary missing from runtime source")
    overlay = int(base[1], 16)
    mapped_data, mapped_functions = set(), []
    rows = []
    catalog = bridge_map.read_text()
    for line in catalog.splitlines():
        if not line.lstrip().startswith("{"):
            continue
        row = re.fullmatch(r'\s*\{ (0x[0-9a-fA-F]+)u, (\d+)u, ([01]), "(\w+)" \},\s*', line)
        if not row:
            raise ValueError(f"invalid battle symbol map row: {line}")
        address, size, function, name = int(row[1], 16), int(row[2]), int(row[3]), row[4]
        rows.append((address, size, function, name))
        if address < overlay:
            if function:
                mapped_functions.append((address, name))
            else:
                mapped_data.add(name)
    if not mapped_functions or not mapped_data or not sizes:
        raise ValueError("empty battle data, functions or size table")

    required = set(evidence["required"]) | (mapped_data & exports.keys())
    linux = {}
    if linux_elf:
        symbols = subprocess.check_output([tool("llvm-nm"), "-DPg", "--defined-only", linux_elf], text=True)
        linux = {parts[0]: parts[1] for row in symbols.splitlines() if len(parts := row.split()) >= 2}
        required |= mapped_data & linux.keys()
    missing = required - (exports.keys() & sizes.keys())
    if missing:
        raise ValueError("missing compiled PE data bindings: " + ", ".join(sorted(missing)))
    locations = linked_symbols(link_map, required | required_callbacks.keys() | {SIZE_TABLE, "g_BattleBridgeSymbols"}, full_owner=True)

    def table_rva(name, owner, expected_size):
        found = locations.get(name, set())
        if len(found) == 1:
            rva, start, length, obj = next(iter(found))
            if Path(obj).name == owner and start <= rva and rva + expected_size <= start + length:
                return rva
        raise ValueError(f"{name} missing or outside {owner}")

    # Match every ordered row and the NUL-terminated string pointed to by the actual PE.
    cursor = table_rva(SIZE_TABLE, "battle_win32_host.c.o", 16 * len(sizes))
    for name, record in sorted(sizes.items()):
        pointer, size = struct.unpack("<QQ", image.contents(cursor, 16))
        text = name.encode("ascii") + b"\0"
        if size != record["size"] or image.contents(pointer - image.base, len(text)) != text:
            raise ValueError(f"compiled size table disagrees with COFF metadata at {name}")
        cursor += 16

    # Current bridge catalogs use an inline fixed-width name. Earlier source used a pointer.
    inline = re.search(r"\bchar name\[(\d+)\];", catalog)
    if inline:
        width = int(inline[1])
        stride = (9 + width + 3) & ~3
    elif "static const PcPortBattleSymbol g_BattleBridgeSymbols[]" in catalog:
        stride = 24
    else:
        raise ValueError("unknown battle bridge catalog representation")
    cursor = table_rva("g_BattleBridgeSymbols", "battle_mips_runtime.c.o", stride * len(rows))
    for address, size, function, name in rows:
        record = image.contents(cursor, stride)
        text = name.encode("ascii") + b"\0"
        if inline:
            name_matches = len(text) <= width and record[9:9 + width] == text.ljust(width, b"\0")
        else:
            pointer = struct.unpack_from("<Q", record, 16)[0]
            name_matches = image.contents(pointer - image.base, len(text)) == text
        if struct.unpack_from("<IIB", record) != (address, size, function) or not name_matches:
            raise ValueError(f"compiled bridge map disagrees with {bridge_map.name} at {address:#x} {name}")
        cursor += stride

    for name in sorted(required):
        rva, size = exports[name], sizes[name]["size"]
        if size <= 0 or not any(start <= rva and rva + size <= start + length for start, length, _, _ in image.data):
            raise ValueError(f"{name} exceeds its PE data section")
        for actual, start, length, owner in locations.get(name, ()):
            if (actual == rva and sizes[name]["origins"].get(Path(owner).name) == rva - start and
                    start <= rva and rva + size <= start + length):
                break
        else:
            raise ValueError(f"{name} export does not match any proven COFF object offset and extent")

    callbacks = callback_owners(required_callbacks, exports, locations, image)
    result = {"compiled_bridge_rows": len(rows), "compiled_data_sizes": len(sizes),
              "verified_data_exports": len(required), "selected_callbacks": callbacks}
    if linux_elf:
        # The guest names of the retail trig entries are deliberately inverted by the runtime.
        trig = {0x8003f8b0: "rsin", 0x8003f8cc: "rcos"}
        checks = 0
        for address, catalog_name in mapped_functions:
            name = trig.get(address, catalog_name)
            if name in linux:
                if linux[name] not in ("T", "W") or not image.executable(exports.get(name, -1)):
                    raise ValueError(f"Linux function {address:#x} {name} lacks a corresponding PE code export")
                checks += 1
        if not checks:
            raise ValueError("no Linux function exports found in the bridge catalog")
        result["linux_owned_function_lookups"] = checks
        result["main_image_function_rows"] = len(mapped_functions)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("pe", "map", "bridge-map", "evidence", "source-root"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--linux-elf", type=Path)
    parser.add_argument("--callback-table", required=True, type=Path,
                        help="JSON object mapping each selected PE export name to its COFF object basename")
    args = parser.parse_args()
    try:
        def unique(pairs):
            result = dict(pairs)
            if len(result) != len(pairs):
                raise ValueError("duplicate callback selection key")
            return result
        callbacks = json.loads(args.callback_table.read_text(), object_pairs_hook=unique)
        if not isinstance(callbacks, dict):
            raise ValueError("expected callback selection object")
        result = verify(args.pe, args.map, args.bridge_map, args.evidence, args.source_root, args.linux_elf,
                        required_callbacks=callbacks)
    except (ValueError, OSError, struct.error, subprocess.CalledProcessError) as error:
        parser.exit(1, f"binding audit failed: {error}\n")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
