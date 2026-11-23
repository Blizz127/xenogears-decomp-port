#!/usr/bin/env python3
"""Generate Windows bridge capacities from unstripped AMD64 game objects."""
import argparse
import json
from collections import defaultdict
from dataclasses import dataclass
import os
from pathlib import Path
import re
import subprocess
import tempfile

C_NAME = r"[A-Za-z_][A-Za-z_0-9]*"
C_INT = r"(?:0[xX][0-9a-fA-F]+|0|[1-9][0-9]*)"
ALIAS = re.compile(rf"\s*XG_(?:SIZED|BRIDGE)_DATA_ALIAS\(\s*({C_NAME})\s*,\s*({C_NAME})\s*,\s*({C_INT})\s*,\s*({C_INT})\s*\)\s*;\s*")
REQUIRED = {
    "g_GameState", "D_80050108", "D_8005010C", "D_80059310", "D_80059314", "D_80059478",
    "D_80059308", "D_8005930C", "D_800595C0", "D_80050104", "D_800500F8", "D_800500FC",
    "D_80050100", "D_80059578",
}


@dataclass
class Definition:
    section: int
    offset: int
    external: bool
    size: int


def read_symbols(inspector, objects):
    if len({p.name for p in objects}) != len(objects):
        raise ValueError("object basenames must be unique in the supplied link list")
    rows = subprocess.check_output([inspector, *objects], text=True)
    symbols = {}
    for row in rows.splitlines():
        obj, name, section, offset, external, size = row.split("\t")
        key = (obj, name)
        if key in symbols:
            raise ValueError(f"duplicate definition {key}")
        symbols[key] = Definition(int(section), int(offset), bool(int(external)), int(size))
    return symbols


def add_aliases(symbols, paths):
    for path in paths:
        obj = path.name + ".o"
        for line_no, line in enumerate(path.read_text().splitlines(), 1):
            if not line.lstrip().startswith(("XG_SIZED_DATA_ALIAS", "XG_BRIDGE_DATA_ALIAS")):
                continue
            match = ALIAS.fullmatch(line)
            if not match:
                raise ValueError(f"{path}:{line_no}: unsupported sized alias declaration")
            name, backing_name, offset_text, size_text = match.groups()
            alias = symbols.get((obj, name))
            backing = symbols.get((obj, backing_name))
            offset = int(offset_text, 0)
            size = int(size_text, 0)
            if (not backing or not alias or not alias.external or alias.section != backing.section
                    or alias.offset != backing.offset + offset or not size or offset + size > backing.size
                    or alias.size not in (0, size)):
                raise ValueError(f"{path}:{line_no}: {name} disagrees with target object or exceeds {backing_name}")
            alias.size = size


def write_table(definitions, output, allow_partial, evidence=None):
    candidates = defaultdict(set)
    for (_, name), symbol in definitions.items():
        if symbol.external:
            candidates[name].add(symbol.size)

    sizes = {name: next(iter(values)) for name, values in candidates.items()
             if len(values) == 1 and 0 not in values}
    if not sizes:
        raise ValueError("no public data definitions with a verified target extent")
    missing = REQUIRED - sizes.keys()
    if missing and not allow_partial:
        raise ValueError("missing mandatory Windows bindings: " + ", ".join(sorted(missing)))

    with tempfile.NamedTemporaryFile(mode="w", dir=output.parent, delete=False,
                                     prefix=output.name + ".") as tmp:
        for name, size in sorted(sizes.items()):
            tmp.write(f'{{"{name}", {size}}},\n')
    os.replace(tmp.name, output)
    if evidence:
        origins = {name: {} for name in sizes}
        for (obj, name), symbol in definitions.items():
            if name in sizes and symbol.external and symbol.size == sizes[name]:
                origins[name][obj] = symbol.offset
        metadata = {
            "required": sorted(REQUIRED),
            "symbols": {name: {"size": sizes[name], "origins": origins[name]} for name in sorted(sizes)},
        }
        with tempfile.NamedTemporaryFile(mode="w", dir=evidence.parent, delete=False,
                                         prefix=evidence.name + ".") as tmp:
            json.dump(metadata, tmp, sort_keys=True)
            tmp.write("\n")
        os.replace(tmp.name, evidence)
    print(f"{output}: {len(sizes)} proven sizes from {len(candidates)} external data names")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inspector", type=Path, required=True, help="host-built coff_data_records")
    parser.add_argument("--alias-source", type=Path, action="append", default=[],
                        help="a source TU using XG_SIZED_DATA_ALIAS")
    parser.add_argument("--pe-evidence", type=Path, help="write audited COFF data offsets for the final linked-image check")
    parser.add_argument("--allow-partial", action="store_true",
                        help="permit absent mandatory rows for fixtures and incomplete builds")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("objects", type=Path, nargs="+", help="actual game link objects, excluding the lookup-table TU")
    args = parser.parse_args()
    records = read_symbols(args.inspector.resolve(), args.objects)
    add_aliases(records, args.alias_source)
    write_table(records, args.output, args.allow_partial, args.pe_evidence)


if __name__ == "__main__":
    main()
