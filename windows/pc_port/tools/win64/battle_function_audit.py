"""Check Win64 battle lookup ownership against the installed r7.21 Linux game."""
from collections import Counter
import hashlib
from pathlib import Path
import re
import struct

from battle_binding_audit import main_image_bridge_rows

# The digest covers ordered VA, catalog name, resolved name and provider for
# all 754 probes in the installed Linux process. It also pins all 138 misses.
LINUX_POLICY = "e2ede6f909b3f9a5c5cde469522ea144b2097a71caef6a607cceddeae6260bd0"
TOOLS = Path(__file__).resolve().parent


def linux_libc_policy(path):
    rows = {}
    for line in path.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        match = re.fullmatch(r"(0x[0-9a-f]{8})\t([A-Za-z_]\w*)", line)
        if not match:
            raise ValueError(f"malformed Linux libc provenance row: {line}")
        va = int(match[1], 16)
        if va in rows:
            raise ValueError(f"duplicate Linux libc VA {va:#x}")
        rows[va] = match[2]
    return rows


def windows_crt_services(path):
    rows = {}
    # Permit explanatory C comments, but never silently exclude unknown rows.
    text = re.sub(r"/\*.*?\*/|//[^\n]*", "", path.read_text(), flags=re.DOTALL)
    for line in text.splitlines():
        if not line.strip():
            continue
        match = re.fullmatch(r"\s*XENO_WIN32_CRT\(\s*(0x[0-9A-Fa-f]+)u?\s*,\s*([A-Za-z_]\w*)\s*\)\s*", line)
        if not match:
            raise ValueError(f"malformed Windows guest CRT service row: {line}")
        va = int(match[1], 16)
        if va in rows:
            raise ValueError(f"duplicate Windows guest CRT VA {va:#x}")
        rows[va] = match[2]
    return rows


def executable_sections(binary):
    image = binary.read_bytes()
    header = struct.unpack_from("<I", image, 60)[0]
    count = struct.unpack_from("<H", image, header + 6)[0]
    size = struct.unpack_from("<H", image, header + 20)[0]
    sections = []
    for index in range(count):
        entry = header + 24 + size + index * 40
        length, rva = struct.unpack_from("<II", image, entry + 8)
        flags = struct.unpack_from("<I", image, entry + 36)[0]
        if flags & 0x20000000:
            sections.append((rva, rva + length))
    return sections


def verify(binary, exports, linux_exports, bridge_map, services=None):
    source = TOOLS.parents[1] / "src"
    external = linux_libc_policy(TOOLS / "r721_libc_functions.tsv")
    registered = windows_crt_services(services or source / "battle_win32_crt_services.inc")
    if registered != external:
        differences = sorted(set(registered.items()) ^ set(external.items()))
        raise ValueError("Windows guest CRT registrations differ from actual Linux providers: " +
                         ", ".join(f"{va:#x} {name}" for va, name in differences))
    sections = executable_sections(binary)
    runtime = source / "battle_mips_runtime.c"
    rows = main_image_bridge_rows(bridge_map, runtime)
    # Follow the production lookup spelling. The verified Linux digest catches
    # changes to either address or name, including swapped retail trig labels.
    override_rows = re.findall(
        r'if\s*\(symbol->is_function\s*&&\s*symbol->address\s*==\s*(0x[0-9A-Fa-f]+)u\)\s*'
        r'host\s*=\s*dlsym\(RTLD_DEFAULT,\s*"(\w+)"\);', runtime.read_text())
    overrides = {int(address, 16): name for address, name in override_rows}
    if len(overrides) != len(override_rows):
        raise ValueError("duplicate function lookup override for a guest address")
    policy = []
    counts = Counter()
    for va, function, catalog_name in sorted(rows):
        if not function:
            continue
        name = overrides.get(va, catalog_name)
        if name in linux_exports:
            if linux_exports[name] not in {"T", "W"}:
                raise ValueError(f"Linux bridge function {name} has unexpected ELF symbol type")
            if va in external:
                raise ValueError(f"Linux ELF and libc both claim guest function {va:#x}")
            rva = exports.get(name)
            if rva is None or not any(start <= rva < end for start, end in sections):
                raise ValueError(f"Linux game function {va:#x} {name} lacks an executable PE export")
            owner = "game"
        else:
            if name in exports:
                raise ValueError(f"unexpected PE export for non-game guest function {va:#x} {name}")
            owner = "libc" if external.get(va) == name else "unresolved"
        policy.append(f"0x{va:08x}\t{catalog_name}\t{name}\t{owner}\n")
        counts[owner] += 1
    if hashlib.sha256("".join(policy).encode("ascii")).hexdigest() != LINUX_POLICY:
        raise ValueError(f"battle function lookups differ from observed r7.21 Linux policy: {dict(counts)}")
    return (f"Verified {len(policy)} main-image battle function lookups: "
            f"{counts['game']} project, {counts['libc']} explicit CRT, {counts['unresolved']} unresolved")
