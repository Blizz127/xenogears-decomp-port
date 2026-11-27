"""Check Win64 link-map contributions against the actual PE compact GPU address range."""
import re
import struct
from pathlib import Path


LIMIT = 0xffffff  # 0 and 0xffffff are end-of-chain sentinels
REQUIRED = {"g_PsxRam": "w", "g_FieldRenderContexts": "w", "xenoForwardOtEnd": "r"}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def image_layout(binary):
    image = Path(binary).read_bytes()
    require(image[:2] == b"MZ", "missing DOS header")
    pe = struct.unpack_from("<I", image, 60)[0]
    require(image[pe:pe+4] == b"PE\0\0", "missing PE signature")
    machine, count, _, _, _, opt_size, flags = struct.unpack_from("<HHIIIHH", image, pe+4)
    opt = pe+24
    require(machine == 0x8664 and struct.unpack_from("<H", image, opt)[0] == 0x20b, "not AMD64 PE32+")
    base = struct.unpack_from("<Q", image, opt+24)[0]
    size = struct.unpack_from("<I", image, opt+56)[0]
    dll_flags = struct.unpack_from("<H", image, opt+70)[0]
    relocations = struct.unpack_from("<II", image, opt+112+8*5)
    require(flags & 1 and not dll_flags & 0x60 and relocations == (0, 0), "PE is relocatable")
    require(0 < base < LIMIT and base+size <= 1 << 32, "PE address range truncates game pointers")
    sections = {}
    for i in range(count):
        entry = opt + opt_size + 40*i
        name, length, rva = struct.unpack_from("<8sII", image, entry)
        protection = struct.unpack_from("<I", image, entry+36)[0]
        sections[name.rstrip(b"\0").decode()] = rva, length, protection
    return base, size, sections


def check(binary, map_path, projects):
    base, size, sections = image_layout(binary)
    projects = {Path(path).resolve() for path in projects}
    seen, writable, named, mapped = set(), [], {}, {}
    output = owner = None
    for line in Path(map_path).read_text().splitlines()[1:]:
        row = re.fullmatch(r"([\da-fA-F]+)\s+([\da-fA-F]+)\s+\d+ (\s*)(.+)", line)
        if not row:
            continue
        rva, length = (int(value, 16) for value in row.group(1, 2))
        indent, label = row.group(3, 4)
        if not indent:
            output, owner = label, None
            mapped[output] = (rva, length)
        elif origin := re.fullmatch(r"(.+):\(([^()]*)\)", label):
            obj, section = origin.groups()
            owner = rva, rva+length, output
            # The driver passes absolute object paths. Do not count a different
            # build's same-named object as our project's linked contribution.
            if Path(obj) in projects:
                seen.add(Path(obj))
                if length and (output == ".data" or section.split("$")[0] in (".bss", ".data", ".sdata", ".sbss")):
                    writable.append((rva, rva+length, obj, section))
        elif label in REQUIRED and owner and owner[0] <= rva < owner[1]:
            named.setdefault(label, set()).add((rva, owner[1], owner[2]))

    for name in (".text", ".rdata", ".data"):
        require(name in sections and mapped.get(name) == sections[name][:2], f"{name} differs between map and PE")
    require(projects and seen == projects, f"missing project objects in map: {sorted(projects-seen)[:8]}")
    require(writable, "no writable project contributions in map")
    for start, end, obj, section in writable:
        require(0 < base+start < base+end <= LIMIT,
                f"compact GPU range exceeded: {obj} {section} [{base+start:#x},{base+end:#x})")
    for name, mode in REQUIRED.items():
        locations = named.get(name, set())
        require(len(locations) == 1, f"{name}: cannot identify unique linked range {locations}")
        start, end, section = next(iter(locations))
        require(bool(sections[section][2] & 0x80000000) == (mode == "w"), f"{name}: wrong PE protection")
        require(0 < base+start < base+end <= LIMIT, f"{name}: compact GPU range exceeded")
    high = max(base+end for _, end, _, _ in writable)
    return (f"PE fixed at {base:#x}, image end {base+size:#x}; {len(writable)} project data "
            f"contributions end at or below {high:#x}; {len(named)} named packet ranges pass")
