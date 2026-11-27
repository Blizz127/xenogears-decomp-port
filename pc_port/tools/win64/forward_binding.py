#!/usr/bin/env python3
"""Generate Windows bridge capacities from unstripped AMD64 game objects."""
import json
from collections import defaultdict
from dataclasses import dataclass
import os
from pathlib import Path
import re
import shlex
import shutil
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


def derive_bindings(source_root: Path, generated_root: Path, object_dir: Path, object_names: list[str]):
    """Measure target data extents from this build before compiling its lookup object."""
    root = Path(source_root).resolve(strict=True)
    output = Path(generated_root).resolve(strict=True)
    objdir = Path(object_dir).resolve(strict=True)
    if not output.is_dir() or not objdir.is_dir():
        raise ValueError("expected existing object and generated include directories")
    names = set(object_names)
    if (not names or len(names) != len(object_names) or "battle_win32_host.c.o" in names
            or not {"port_main.o", "stubs.o"} <= names or not any(n.startswith("battle_host_") for n in names)):
        raise ValueError("expected the ordered game roster without the binding lookup object")
    objects = []
    for name in object_names:
        if not re.fullmatch(r"[A-Za-z0-9_.-]+[.]o", name):
            raise ValueError(f"invalid object basename {name!r}")
        obj = objdir / name
        if obj.is_symlink() or not obj.is_file() or obj.resolve(strict=True).parent != objdir:
            raise ValueError(f"missing or redirected fresh object: {name}")
        objects.append(obj)
    table = output / "battle_win32_binding_sizes.inc"
    evidence = output / "battle_win32_binding_evidence.json"
    if table.exists() or evidence.exists():
        raise ValueError("refusing a generated directory containing an earlier Windows binding result")

    tools = Path(__file__).resolve().parent
    cxx = shutil.which("clang++-21")
    config = shutil.which("llvm-config-21")
    if not cxx or not config:
        raise ValueError("Windows target binding generation requires clang++-21 and llvm-config-21 on PATH")
    srcdir = root / "pc_port/src"
    if not srcdir.is_dir():
        raise ValueError("missing project host source directory")
    flags = subprocess.check_output(
        [config, "--cxxflags", "--ldflags", "--libs", "object", "debuginfodwarf",
         "support", "--system-libs"], text=True)
    with tempfile.TemporaryDirectory(prefix="coff-bind-", dir=output) as scratch:
        temp = Path(scratch)
        inspector = temp / "bind_coff"
        command = [cxx, str(tools / "forward_bind_coff.cpp"), *shlex.split(flags), "-O2", "-o", str(inspector)]
        subprocess.run(command, check=True)
        definitions = read_symbols(inspector, objects)
        alias_sources = sorted(path for path in srcdir.rglob("*.c") if path.name + ".o" in names)
        add_aliases(definitions, alias_sources)
        write_table(definitions, temp / table.name, False, temp / evidence.name)
        os.replace(temp / evidence.name, evidence)
        os.replace(temp / table.name, table)
    return table, evidence
