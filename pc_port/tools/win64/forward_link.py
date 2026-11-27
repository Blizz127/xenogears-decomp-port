#!/usr/bin/env python3
"""Link and audit the Win64 hybrid game from freshly compiled production objects."""
import argparse
import hashlib
import json
import os
import re
import shlex
import shutil
import struct
import subprocess
import sys
from pathlib import Path

from forward_binding import derive_bindings
from forward_compile import RENDERER_SOURCE_FILES, VENDOR, verify_renderer_donor
from forward_bind_verify import Image, linked_symbols, verify as verify_binding
from forward_compact import check as check_compact

SYSTEM_LIBS = ("opengl32", "user32", "bcrypt", "gdi32", "winmm", "ws2_32", "ole32",
               "shell32", "imm32", "version", "setupapi", "advapi32")
ELF_ONLY = {"_IO_stdin_used", "__bss_start", "__data_start", "_edata", "_end", "_start",
            "data_start", "__wrap_acosf", "__wrap_asinf", "__wrap_sqrtf"}
LINK_ONLY = {"PcPort_RunTitleMenu", "PcPort_SkipFieldMovie", "PcPort_SkipMovieState"}
GPU_EXPORTS = {"AmdPowerXpressRequestHighPerformance", "NvOptimusEnablement"}
ALIASES = {"EnterCriticalSection": "XenoPsyQEnterCriticalSection"}
HOST_OBJECT = "battle_win32_host.c.o"
BASE = 0x100000
ABI_SHA256 = "0eeef93e1c803594427a59030c9591188462d09b8d54da0b20410c913d974c6c"
HOLES = {245, 5950}
RETIRED_SHA256 = "d836e8679f46085f48f388eae3dd3e75be9346124677705705a37bda46f10fa6"
FIELD_MENU = {"PcPort_RoboMenuDispatch": 2363, "PcPort_RoboMenuFieldOuterAvailable": 2373,
              "PcPort_RoboMenuFieldVisitPending": 2375, "PcPort_RoboMenuFieldVisitPoll": 2376,
              "PcPort_RoboMenuFieldVisitRetry": 2378}
ORDINARY_MENU = {"func_801C58EC":5838, "func_801C62A8":5858, "func_801C7BF4":5872,
                 "func_801C7D78":5873, "func_801E4258":6182, "func_801E42AC":6183,
                 "func_801E433C":6184, "func_801E4754":6185, "func_801E4928":6186}
# These v0.7.93-preview exports survive the changes to the File1, Results, runtime and movie providers.
MODULE_BOUNDARY = {
    ("battle_guest_trampolines.c.o", "T"): {
        3496, 3570, 4308, 4312, 4313, 4315, 4316, 4332, 4333, 4334,
        4432, 4439, 4642, 4668, 4669, 4671, 4672, 4673, 4679, 4681,
        4684, 4689, 4690, 4691, 4693, 4694, 4695, 4700, 4702, 4703,
        4705, 4714, 4756, 5044, 5146, 5599, 5679, 5696, 5738, 5779,
        5786, 5793, 5813, 5825,
    },
    ("src_movie_main.c.o", "T"): {1784},
    ("battle_mips_runtime.c.o", "T"): {1978, 1979, 1980, 1981, 1982, 1983, 1984, 1986, 1987, 1989, 3796, 4273},
    ("battle_mips_runtime.c.o", "B"): {6291, 6292},
    ("battle_results_host.c.o", "T"): {3729, 3730, 3745, 3747, 3756, 3759, 3761, 3768, 3769, 3770,
                                      3771, 3772, 3773, 3774, 3775, 3776},
}
IMPORTS = {"libSDL2.dll.a": "29def8cd8385ffbbf4e7ac6e47359d1213b70c6aed0c282e809ca35bd33c9b37",
           "libOpenAL32.dll.a": "e9fe9cd8e500774102fc4a51d55959eefabd451e7415651e1ef75fd7fa5b15ef"}
DLLS = {"SDL2.dll": "a2648c887820d882805d00c40c30181a21dbb083f24cfe7ab6cc3e451a55a3b6",
        "OpenAL32.dll": "6aa458bc42bc04683126d40f94c853895f953387587a56cb29d61de0a70c1aee"}



def require(ok, message):
    if not ok:
        raise ValueError(message)


def llvm_tool(name):
    for candidate in ("llvm-" + name + "-21",):
        tool = shutil.which(candidate)
        if tool:
            return tool
    raise FileNotFoundError("llvm-" + name)


def read(*args):
    return subprocess.check_output(args, text=True)


def write_json(path, data):
    path.write_text(json.dumps(data, indent=2) + "\n")


def fingerprint(path):
    with path.open("rb") as stream:
        return {"path": str(path), "size": path.stat().st_size,
                "sha256": hashlib.file_digest(stream, "sha256").hexdigest()}


def symbol_types(paths, dynamic=False):
    cmd = [llvm_tool("nm"), "--extern-only", "--defined-only", "--format=posix"]
    if dynamic:
        cmd.append("--dynamic")
    result = {}
    for i in range(0, len(paths), 100):
        for line in read(*cmd, *paths[i:i+100]).splitlines():
            row = re.fullmatch(r"([A-Za-z_]\w*) ([TDBRSGCVW]) [0-9a-fA-F]+ [0-9a-fA-F]+", line)
            if row and (row[2] != "W" or row[1] not in result):
                result[row[1]] = row[2]
    return result


def crt_options(zig, probe, base):
    # Ask this installed compiler to resolve MinGW's startup object, UCRT imports
    # and compiler runtime for a real program under the same target parameters.
    probe.mkdir(parents=True, exist_ok=True)
    (probe / "probe.c").write_text("int main(void) { return 0; }\n")
    compile_cmd = [zig, "cc", "-target", "x86_64-windows-gnu", "-O2", "-c", "probe.c", "-o", "probe.o"]
    subprocess.run(compile_cmd, cwd=probe, check=True)
    link_cmd = [zig, "build-exe", "-OReleaseFast", "-target", "x86_64-windows-gnu", "--verbose-link",
                "--image-base", str(base), "-lc", *["-l" + lib for lib in SYSTEM_LIBS],
                "probe.o", "-femit-bin=probe.exe"]
    result = subprocess.run(link_cmd, cwd=probe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (probe / "link.log").write_text(result.stdout)
    result.check_returncode()
    rows = [shlex.split(line) for line in result.stdout.splitlines() if line.startswith("lld-link ")]
    if len(rows) != 1:
        raise ValueError("cannot identify MinGW linker invocation in " + str(probe / "link.log"))
    row = rows[0]
    dividing = row.index("-lldmingw")
    options = [s for s in row[1:dividing] if s.startswith("-") and
               not s.upper().startswith(("-OUT:", "-PDB", "-DEBUG", "-IMPLIB:"))]
    runtime = row[dividing:]
    if not any(Path(s).name == "crt2.obj" for s in runtime):
        raise ValueError("MinGW CRT startup object absent from probe link")
    write_json(probe / "commands.json", {"compile": compile_cmd, "link": link_cmd,
                                         "observed": row, "options": options, "runtime": runtime})
    return options, runtime


def pe_exports(image, *, holes=()):
    text = read(llvm_tool("readobj"), "--coff-exports", image)
    rows = re.findall(r"^Export \{\n  Ordinal: (\d+)\n  Name: *([^\s]*)\n  RVA: (0x[\da-fA-F]+)\n\}$", text, re.M)
    if not rows or len(rows) != text.count("Export {"):
        raise ValueError(f"incomplete COFF export readback: {image}")
    result = {}
    observed = set()
    vacant = set()
    for ordinal_text, name, addr in rows:
        ordinal, rva = int(ordinal_text), int(addr, 16)
        if ordinal in observed:
            raise ValueError(f"duplicate PE ordinal {ordinal}: {image}")
        observed.add(ordinal)
        if name and rva and name not in result:
            result[name] = (ordinal, rva)
        elif not name and not rva:
            vacant.add(ordinal)
        else:
            raise ValueError(f"duplicate, null named, or unnamed nonzero export {ordinal}: {image}")
    if observed != set(range(1, max(observed) + 1)) or vacant != set(holes):
        raise ValueError(f"unexpected or unreported PE ordinal holes {sorted(vacant)}: {image}")
    return result


def audit_pe(image, ordinals, wraps, holes=()):
    actual = pe_exports(image, holes=holes)
    if {name: record[0] for name, record in actual.items()} != ordinals:
        raise ValueError(f"linked names or export ordinals differ from the DEF: {image}")
    data = image.read_bytes()
    pe = struct.unpack_from("<I", data, 60)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    opt_size = struct.unpack_from("<H", data, pe + 20)[0]
    sections = []
    for i in range(count):
        offset = pe + 24 + opt_size + 40*i
        extent, rva = struct.unpack_from("<II", data, offset + 8)
        flags = struct.unpack_from("<I", data, offset + 36)[0]
        if flags & 0x40000000:
            sections.append((rva, rva + extent))
    for name, (_, rva) in actual.items():
        if not any(start <= rva < end for start, end in sections):
            raise ValueError(f"dangling PE export: {name} ({rva:#x})")
    imports = read(llvm_tool("readobj"), "--coff-imports", image)
    imported_symbols = set(re.findall(r"  Symbol: (\w+) \(\d+\)", imports))
    for name in wraps:
        if name in imported_symbols or actual[name][1] == actual["__wrap_" + name][1]:
            raise ValueError(f"SDK and wrapper ownership mismatch: {name}")
    return {"export_count": len(actual), "dlls": sorted(set(re.findall(r"^  Name: (.*)$", imports, re.M)))}


def ordinals():
    """Read the public v0.7.93-preview export ordinals, including reserved holes."""
    table = Path(__file__).with_name("public-ordinals.tsv")
    require(fingerprint(table)["sha256"] == ABI_SHA256, "public ordinal table differs from the recorded ABI")
    names, gaps = {}, set()
    for expected, line in enumerate(table.read_text().splitlines(), 1):
        ordinal, name = line.split("\t")
        require(int(ordinal) == expected, "nonconsecutive ordinal table")
        if name == "-":
            gaps.add(expected)
        else:
            require(re.fullmatch(r"[A-Za-z_]\w*", name) and name not in names, "invalid duplicate export name")
            names[name] = expected
    require(len(names) == 9038 and gaps == HOLES and max(names.values()) == 9040, "public ABI roster incomplete")
    return names


# Public .93 source-to-export spelling for the selected native battle lookups.
SELECTED_BATTLE_EXPORTS = {
    "battle_host_main103.c.o": "func_800B89F4",
    "battle_host_main119.c.o": "func_800BDD34",
    "battle_host_main12.c.o": "func_80079934",
    "battle_host_main13.c.o": "func_80079E18 func_80079E4C func_80079E7C",
    "battle_host_main19.c.o": (
        "func_8007AE38 func_8007AE98 func_8007AF5C func_8007AFAC func_8007AFFC "
        "func_8007B040 func_8007B084 func_8007B0C8 func_8007B134 func_8007B198 "
        "func_8007B208 func_8007B264"
    ),
    "battle_host_main22.c.o": "func_8007B98C func_8007B9C8 func_8007BA88 func_8007BAB8",
    "battle_host_main31.c.o": "func_8007FB70 func_8007FBE0 func_8007FCE8",
    "battle_host_main32.c.o": "func_8007FDEC",
    "battle_host_main33.c.o": "func_8007FEC4 func_800800E8",
    "battle_host_main34.c.o": "func_80080B64 func_80080C6C",
    "battle_host_main4.c.o": "func_80076AC8",
    "battle_host_main41.c.o": (
        "func_80089B50 func_80089BEC func_80089C08 func_80089C24 func_80089C48 "
        "func_80089C6C func_80089C9C"
    ),
    "battle_host_main44.c.o": "func_8008ABB8",
    "battle_host_main5.c.o": "func_80076C34",
    "battle_host_main51.c.o": "func_8009413C func_80094C78 func_800957D8 func_8009795C",
    "battle_host_main52.c.o": "func_80097D08",
    "battle_host_main53.c.o": "func_80099FB0 func_8009A074",
    "battle_host_main55.c.o": "func_8009A7E4 func_8009A854 func_8009A9D0 func_8009AA44 func_8009AB00",
    "battle_host_main56.c.o": "func_8009AB38 func_8009AC48 func_8009ADA0 func_8009AEFC",
    "battle_host_main57.c.o": "func_8009AFD8 func_8009B098",
    "battle_host_main58.c.o": (
        "func_8009B104 func_8009B1E4 func_8009B46C func_8009B684 func_8009BAC4 "
        "func_8009BD94 func_8009BE0C func_8009C050 func_8009C0E0"
    ),
    "battle_host_main69.c.o": (
        "func_800A579C func_800A5870 func_800A5914 func_800A5A48 func_800A5BE8 "
        "func_800A5D54 func_800A5E9C"
    ),
    "battle_host_main7.c.o": "func_80077980",
    "battle_host_main71.c.o": "func_800AA79C func_800AA7DC",
    "battle_host_main7_port.c.o": "battle_func_80077698",
    "battle_host_main90.c.o": "func_800B6930 func_800B6990",
    "battle_host_main91.c.o": "func_800B69E4",
    "battle_host_mainasm_10FF4.c.o": "battle_func_80080AE4",
    "battle_host_mainasm_18B1C.c.o": "battle_func_8008887C",
    "battle_host_mainasm_1AFB0.c.o": "battle_func_8008AAA0",
    "battle_host_mainc107.c.o": "func_800B9258",
    "battle_host_mainc108.c.o": "func_800B9B30",
    "battle_host_mainc124.c.o": "func_800BEDE8",
    "battle_host_mainc129.c.o": "func_800BF730",
    "battle_host_mainc76.c.o": "func_800B16A4",
    "battle_host_mainc99.c.o": "func_800B7C28",
    "battle_host_mainl81.c.o": "func_800B3B6C",
}


def selected_callbacks():
    owners = {name: owner for owner, names in SELECTED_BATTLE_EXPORTS.items() for name in names.split()}
    require(len(owners) == sum(len(names.split()) for names in SELECTED_BATTLE_EXPORTS.values()) == 91
            and len(set(owners.values())) == 37, "incomplete or duplicate public battle lookup bindings")
    return owners


def retired_ordinals(prior):
    """Verify names whose public ordinal slots remain vacant in this Windows profile.

    The v0.7.93-preview ABI reserved slots 245 and 5950. This profile also
    reserves the 1,069 optional Gear Shop exports, 267 arena exports and
    PcPort_MovieIdct (8222), 1,025 exports from omitted battle objects,
    307 ordinary menu exports, 155 previous File1, Results and runtime exports,
    63 symbols from the movie-state overlay, 19 from retired battle routines and
    158 further ABI entries absent from the compiled game profile. Of these
    historical module removals, 44 public names are now backed by typed guest
    calls. Their Win64 ordinals and wrapper owners are verified separately.
    The World Map preview removes another 611 exports. The field menu,
    retained World capture and terminal interfaces, and 91 selected battle
    lookup functions keep their original slots.
    """
    path = Path(__file__).with_name("retired-exports.tsv")
    require(fingerprint(path)["sha256"] == RETIRED_SHA256, "unexpected Windows export retirement table")
    retired = {}
    for line in path.read_text().splitlines():
        ordinal, name, provider, kind = line.split("\t")
        require(name not in retired and prior.get(name) == int(ordinal) and kind in {"function", "DATA"}
                and provider.endswith(".c"), f"retired symbol disagrees with the public ABI: {name}")
        retired[name] = int(ordinal)
    retained = retained_interfaces(prior)
    require(len(retired) == 3641 and retired.get("PcPort_MovieIdct") == 8222 and
            retired.keys().isdisjoint(retained) and LINK_ONLY.isdisjoint(prior),
            "optional-provider removal conflicts with the retained public interfaces")
    need_callbacks = selected_callbacks()
    require(not retired.keys() & need_callbacks.keys() and need_callbacks.keys() <= prior.keys(),
            "retirements overlap native battle lookup exports")
    return retired


def retained_interfaces(prior):
    interfaces = {}
    for roster, owner in ((FIELD_MENU, "gear_shop_unavailable.c.o"), (ORDINARY_MENU, "src_menu_main_misc.c.o")):
        for name, ordinal in roster.items():
            require(prior.get(name) == ordinal, f"retained public menu ordinal changed: {name}")
            interfaces[name] = (ordinal, owner, "T")
    by_ordinal = {ordinal: name for name, ordinal in prior.items()}
    for (owner, kind), slots in MODULE_BOUNDARY.items():
        for slot in slots:
            name = by_ordinal[slot]
            require(name not in interfaces, f"overlap among retained interfaces: {name}")
            interfaces[name] = (slot, owner, kind)
    return interfaces


def required_interface_types(elf, target, interfaces):
    for name, (_, _, kind) in interfaces.items():
        elf_kind = "W" if name == "PcPort_BattleSharedData" else kind
        require(elf.get(name) == elf_kind and target.get(name) == kind,
                f"retained {kind} interface has a different ELF or COFF definition: {name}")


def check_interface_owners(actual, pe, link_map, interfaces):
    image = Image(pe)
    shared = "PcPort_BattleSharedData"
    pointer = ".refptr." + shared
    locations = linked_symbols(link_map, set(interfaces) | {pointer})
    pointers = {rva for rva, start, size, _ in locations.get(pointer, ()) if start <= rva and rva + 8 <= start + size}
    require(len(pointers) == 1, "optional battle data lookup lacks its single Windows import pointer")
    address = struct.unpack("<Q", image.contents(pointers.pop(), 8))[0]
    require(address == image.base + actual[shared][1], "optional battle data lookup differs from its Windows export")
    for name, (ordinal, expected_owner, kind) in interfaces.items():
        require(actual[name][0] == ordinal, f"retained interface ordinal changed: {name}")
        rva = actual[name][1]
        found = locations.get(name, set())
        require(len(found) == 1, f"retained export lacks a unique physical contribution: {name}")
        addr, start, length, owner = next(iter(found))
        require(addr == rva and start <= rva < start + length and owner == expected_owner, f"retained export lacks its compiled provider: {name}")
        if kind == "B":
            require(any(begin <= rva < begin + extent for begin, extent, _, _ in image.data),
                    f"retained data export is outside PE data: {name}")
        else:
            require(image.executable(rva), f"retained function is outside PE executable sections: {name}")


def discard_optional_null_imports(inputs, symbols, retired):
    """Classify apparent COFF weak definitions before enforcing ordinal retirement."""
    pending = {name for name in retired if symbols.get(name) == "W"}
    if not pending:
        return {}
    dependents = {}
    members = {}
    for start in range(0, len(inputs), 90):
        group = inputs[start:start + 90]
        marks = read(llvm_tool("nm"), "--print-file-name", "--format=posix", "--extern-only",
                     "--defined-only", *group)
        paths = {str(path): path for path in group}
        for line in marks.splitlines():
            unit, sep, fields = line.partition(": ")
            row = fields.split()
            if not sep or len(row) != 4 or row[0] not in pending:
                continue
            require(row[1] == "W", f"retired symbol has executable or data definition: {row[0]}")
            container = unit if unit in paths else re.sub(r"\[[^/\[\]]+\]$", "", unit)
            require(container in paths, f"unknown COFF origin {unit}")
            dependents.setdefault(paths[container], set()).add(row[0])
            members.setdefault(row[0], set()).add(unit)
    evidence = {name: [] for name in pending}
    for path, names in dependents.items():
        dump = read(llvm_tool("readobj"), "--symbols", path)
        for unit in re.split(r"(?m)^File: ", dump)[1:]:
            owner, _, body = unit.partition("\n")
            # llvm-nm uses archive[member] and llvm-readobj uses archive(member).
            if owner != str(path):
                require(owner.startswith(str(path) + "(") and owner.endswith(")"), f"unidentified member {owner}")
                owner = str(path) + "[" + owner[len(str(path)) + 1:-1] + "]"
            records = {}
            for block in body.split("  Symbol {\n")[1:]:
                name_match = re.match(r"    Name: ([^\n]+)\n", block)
                if name_match:
                    records.setdefault(name_match[1], []).append(block.split("\n  }", 1)[0])
            for name in names:
                if owner not in members[name]:
                    continue
                candidates = records.get(name, [])
                require(len(candidates) == 1, f"ambiguous COFF symbol {name} in {owner}")
                entry = candidates[0]
                target = re.search(r"(?m)^      Linked: (.+) \([0-9]+\)$", entry)
                require(target and "Section: IMAGE_SYM_UNDEFINED (0)\n" in entry
                        and "StorageClass: WeakExternal (0x69)\n" in entry and "Search: Alias (0x3)" in entry,
                        f"retired export has a physical or unsupported weak symbol: {name}")
                default = records.get(target[1], [])
                require(len(default) == 1 and "Value: 0\n" in default[0]
                        and "Section: IMAGE_SYM_ABSOLUTE (-1)\n" in default[0],
                        f"retired export still has a non-null COFF provider: {name}")
                evidence[name].append({"object": owner, "null_default": target[1]})
    for name in pending:
        require(evidence[name] and {r["object"] for r in evidence[name]} == members[name],
                f"incomplete weak COFF evidence for {name}")
        del symbols[name]
    return evidence


def make_def(path, prior, elf, target, retired):
    selected = dict(target)
    for public, implementation in ALIASES.items():
        require(implementation in selected, f"missing Windows implementation {implementation}")
        selected[public] = selected[implementation]
    require(GPU_EXPORTS <= selected.keys(), "missing GPU selection exports")
    absent = set(elf) - selected.keys()
    unsupported = sorted(name for name in absent - ELF_ONLY - LINK_ONLY if not name.startswith("_Z"))
    require(not unsupported, f"missing PC game exports in Windows objects: {unsupported[:12]}")
    names = ((set(elf) & selected.keys()) | GPU_EXPORTS) - LINK_ONLY
    for name, ordinal in retired.items():
        require(prior.get(name) == ordinal and name not in elf and name not in target,
                f"retired export still present, or wrong ordinal: {name}")
    gone = sorted(prior.keys() - names - retired.keys())
    require(not gone, f"public ABI symbols missing from fresh target: {gone[:12]}")
    assigned = {name: ordinal for name, ordinal in prior.items() if name not in retired}
    first = max(prior.values()) + 1
    assigned.update({name: first + i for i, name in enumerate(sorted(names - prior.keys()))})
    with path.open("x") as defs:
        print("EXPORTS", file=defs)
        for name in sorted(names):
            symbol = f"{name}={ALIASES[name]}" if name in ALIASES else name
            flag = " DATA" if name in GPU_EXPORTS or elf[name] in "BDRSGCV" else ""
            print(f"  {symbol} @{assigned[name]}{flag}", file=defs)
    return assigned


def verify_pin(path, record):
    actual = fingerprint(path)
    require(record["bytes"] == actual["size"] and record["sha256"] == actual["sha256"],
            f"artifact identity differs: {path.name}")


def local_artifact(base, record, allow_absolute=False):
    ref = Path(record["path"])
    require(ref.parts and ".." not in ref.parts and (allow_absolute or not ref.is_absolute()),
            f"invalid build artifact path: {ref}")
    path = (base / ref).resolve(strict=True)
    require(path.is_relative_to(base) and path.is_file(), f"artifact outside build root: {ref}")
    verify_pin(path, record)
    return path


def file_record(path):
    pin = fingerprint(path)
    return {"path": pin["path"], "bytes": pin["size"], "sha256": pin["sha256"]}


def verify_linux_pin(compilation, base, linuxfile):
    pin = compilation["linux"]
    require((base / pin["path"]).resolve(strict=True) == linuxfile,
            "the selected Linux receipt differs from the one used by the Windows compiler")
    verify_pin(linuxfile, pin)


def generated_bindings(compilation, base, generated, bridge):
    require(generated.is_relative_to(base), "Windows generated directory outside the compile output")
    # All three are actual compiler products. Evidence links the generated size table to its COFF owners.
    names = ("battle_win32_binding_evidence.json", "battle_win32_binding_sizes.inc", "battle_bridge_map.inc")
    for name in names:
        path = local_artifact(base, compilation["generated"][name])
        require(path == (generated / name).resolve(strict=True), f"different generated input selected: {name}")
    bridge_record = compilation["generated"][names[2]]
    verify_pin(bridge, bridge_record)
    consumers = {o["name"]: o for o in compilation["objects"]}
    for name, owner in ((names[1], HOST_OBJECT), (names[2], "battle_mips_runtime.c.o")):
        pin = compilation["generated"][name]
        label = "generated/" + Path(pin["path"]).as_posix()
        require(consumers[owner]["dependencies"].get(label) == pin["sha256"],
                f"compiler dependency for {owner} does not name the selected {name}")
    return generated / names[0]


def ordered_objects(receipt, game_order):
    require(receipt.get("schema") == 1 and receipt.get("mode") == "full", "a full Windows compile receipt is required")
    require(isinstance(game_order, list) and game_order and len(set(game_order)) == len(game_order)
            and all(isinstance(name, str) and name.endswith(".o") and Path(name).name == name for name in game_order)
            and HOST_OBJECT not in game_order and "stubs.o" in game_order, "invalid Linux game object roster")
    require([row["name"] for row in receipt["objects"]] == game_order + [HOST_OBJECT],
            "Windows object order must exactly match the Linux roster followed by the resolver")
    return receipt["objects"]


def renderer_compiler_inputs(receipt, commit, tree, profile_blob, project_digest):
    require(receipt.get("mode") == "full" and receipt["source_commit"] == commit and receipt["source_tree"] == tree
            and receipt["target"] == "x86_64-windows-gnu" and receipt["compiler"] == "0.13.0",
            "full renderer receipt must match the selected source, tree and Windows compiler")
    measured = receipt["compiler_inputs"]
    require(isinstance(measured, dict) and measured, "missing renderer compiler inputs")
    vendor = "pc_port/extern/PsyCross/"
    project_dirs = ("pc_port/src/", "pc_port/include/", "pc_port/include_shim/", "pc_port/renderer_headers/",
                    "src/", "include/")
    suffixes = {".c", ".C", ".cc", ".cpp", ".h", ".hpp", ".inc", ".inl", ".S"}
    for key, digest in measured.items():
        require(isinstance(key, str) and key.startswith((vendor + "src/", vendor + "include/", *project_dirs))
                and all(re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.+-]*", part) for part in key.split("/"))
                and Path(key).suffix in suffixes and isinstance(digest, str) and re.fullmatch(r"[0-9a-f]{64}", digest),
                f"invalid renderer compiler path or digest: {key!r}")
    profile_pin = receipt["profile"]
    require(profile_pin["bytes"] == len(profile_blob) and profile_pin["sha256"] == hashlib.sha256(profile_blob).hexdigest(),
            "renderer receipt does not identify the profile in the selected Git commit")
    profile = json.loads(profile_blob)
    expected = profile["restored_inputs"]
    require(profile["target"] == receipt["target"] and profile["zig"] == receipt["compiler"] and
            measured.keys() == expected.keys(), "full renderer receipt omits or adds profile inputs")
    require(RENDERER_SOURCE_FILES.keys() <= measured.keys() and receipt.get("source_files") == RENDERER_SOURCE_FILES,
            "full renderer receipt must identify all five committed headers")
    for key, value in measured.items():
        if key in RENDERER_SOURCE_FILES:
            actual = project_digest(RENDERER_SOURCE_FILES[key])
        else:
            actual = expected[key] if key.startswith(vendor) else project_digest(key)
        require(value == actual, f"renderer input does not match its restored vendor or Git project source: {key}")
    members, hashes, units, seen = [], {}, {}, set()
    require(isinstance(receipt["units"], list) and receipt["units"], "expected renderer archive member receipts")
    for row in receipt["units"]:
        member, obj_hash, deps = row["member"], row["object_sha256"], row["compiler_inputs"]
        require(isinstance(member, str) and re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.+-]*\.o", member)
                and member not in hashes and isinstance(obj_hash, str) and re.fullmatch(r"[0-9a-f]{64}", obj_hash),
                "invalid or duplicate renderer archive member")
        require(isinstance(deps, list) and deps and all(isinstance(k, str) and k in measured for k in deps)
                and len(set(deps)) == len(deps), f"missing or invalid member dependency list: {member}")
        primary = [k for k in deps if k.startswith(vendor + "src/") and Path(k).name + ".o" == member]
        require(len(primary) == 1, f"renderer unit does not identify its primary source: {member}")
        require(member in profile["unit_inputs"] and set(deps) == set(profile["unit_inputs"][member]) & expected.keys(),
                f"renderer member dependencies differ from the committed profile: {member}")
        members.append(member)
        hashes[member] = obj_hash
        units[member] = deps
        seen.update(deps)
    profile_members = [Path(source).name + ".o" for source in profile["sources"]]
    require(members == profile_members and set(members) == profile["unit_inputs"].keys(),
            "full renderer archive member roster differs from its committed profile")
    require(seen == measured.keys(), "per-member dependency union differs from measured renderer inputs")
    return {"platform": "windows", "source_commit": commit, "source_tree": tree, "sdk_sha256": receipt["archive_sha256"],
            "compiler_inputs": measured, "source_files": dict(RENDERER_SOURCE_FILES),
            "members": members, "member_sha256": hashes, "units": units}


def sdk_library(sdk, receipt, commit, tree, profile_blob, project_digest):
    proof = renderer_compiler_inputs(receipt, commit, tree, profile_blob, project_digest)
    require(fingerprint(sdk)["sha256"] == proof["sdk_sha256"], "renderer archive hash mismatch")
    require(read(llvm_tool("ar"), "t", sdk).splitlines() == proof["members"],
            "renderer member identity/order differs from its compiler receipt")
    for member in proof["members"]:
        data = subprocess.check_output([llvm_tool("ar"), "p", sdk, member])
        require(len(data) >= 20 and data.startswith(b"\x64\x86") and hashlib.sha256(data).hexdigest() == proof["member_sha256"][member],
                f"renderer object differs from its compiler receipt: {member}")
    return proof


def verify_game_renderer_headers(rows, renderer_inputs):
    for key in RENDERER_SOURCE_FILES:
        dependency = ("renderer/" + key.removeprefix(VENDOR)) if key.startswith(VENDOR) else ("source/" + key)
        for row in rows:
            deps = row["dependencies"]
            if dependency in deps:
                require(deps[dependency] == renderer_inputs["compiler_inputs"][key],
                        f"game and renderer header differ for {row['name']}: {key}")


def linux_inputs(record, base, commit, tree):
    require(record.get("source") == commit and record.get("tree") == tree, "Linux and Windows source/tree differ")
    elf, bridge = (local_artifact(base, record[k], allow_absolute=True) for k in ("elf", "bridge"))
    generated_bridge = local_artifact(base, record["generated"]["battle_bridge_map.inc"], allow_absolute=True)
    require(bridge == generated_bridge, "Linux receipt identifies different generated and linked bridge maps")
    with elf.open("rb") as stream:
        header = stream.read(20)
    require(header[:6] == b"\x7fELF\x02\x01" and len(header) == 20 and struct.unpack_from("<HH", header, 16) == (2,62),
            "expected a linked AMD64 Linux executable")
    return elf, bridge


def link(args):
    objfile = args.objects.resolve(strict=True)
    linuxfile = args.linux.resolve(strict=True)
    sdkfile = args.sdk_receipt.resolve(strict=True)
    src = args.source_root.resolve(strict=True)
    gen = (objfile.parent / args.generated).resolve(strict=True)
    sdk = args.sdk.resolve(strict=True)
    prefix = args.prefix.resolve(strict=True)
    objects_record, linux, renderer = (json.loads(p.read_bytes()) for p in (objfile, linuxfile, sdkfile))
    require(objects_record.get("mode") == "full", "a full Windows compile receipt is required")
    commit, tree = objects_record["source"], objects_record["tree"]
    require(re.fullmatch(r"[0-9a-f]{40}", commit) and re.fullmatch(r"[0-9a-f]{40}", tree), "invalid source identity")
    require(read("git", "-C", src, "rev-parse", "HEAD", "HEAD^{tree}").splitlines() == [commit, tree],
            "source checkout differs from compile receipts")
    verify_linux_pin(objects_record, objfile.parent, linuxfile)
    elf, bridge = linux_inputs(linux, linuxfile.parent, commit, tree)
    rows = ordered_objects(objects_record, linux["game_order"])
    objdir = objfile.parent
    objects = [local_artifact(objdir, r) for r in rows]
    for obj, row in zip(objects, rows):
        require(obj.name == row["name"] and re.fullmatch(r"[a-f0-9]{64}", row["source_sha256"]) and row["source"],
                f"missing source identity for {obj.name}")
        with obj.open("rb") as stream:
            require(stream.read(2) == b"\x64\x86", f"expected AMD64 COFF game object: {obj.name}")
    def source_bytes(key):
        return subprocess.check_output(["git", "--no-replace-objects", "-C", src, "show", f"{commit}:{key}"])
    profile_blob = source_bytes("pc_port/tools/win64/renderer-profile.json")
    verify_renderer_donor(json.loads(profile_blob))
    renderer_inputs = sdk_library(sdk, renderer, commit, tree, profile_blob,
                                  lambda key: hashlib.sha256(source_bytes(key)).hexdigest())
    verify_game_renderer_headers(rows, renderer_inputs)
    libs = [sdk]
    for name, digest in IMPORTS.items():
        path = prefix / "lib" / name
        require(fingerprint(path)["sha256"] == digest, f"import library differs from the SDK dependency pin: {name}")
        libs.append(path)
    for name, digest in DLLS.items():
        require(fingerprint(prefix / "bin" / name)["sha256"] == digest, f"runtime dependency differs from its pin: {name}")
    evidence = generated_bindings(objects_record, objdir, gen, bridge)
    sdk_vars = symbol_types([*objects, sdk])
    wraps = [w for w in linux["wraps"] if w not in {"acosf","asinf","sqrtf"}]
    require(wraps and len(wraps) == len(set(wraps)) and all(re.fullmatch(r"[A-Za-z_]\w*", w) and w in sdk_vars and
            "__wrap_" + w in sdk_vars for w in wraps), "Linux wrappers lack distinct native and wrapped Windows symbols")
    require(read(args.zig, "version").strip() == "0.13.0", "Zig 0.13.0 required")
    out = args.out.resolve()
    require(not out.exists() or (out.is_dir() and not any(out.iterdir())), "expected a fresh Windows link output")
    out.mkdir(parents=True, exist_ok=True)
    prior = ordinals()
    retired = retired_ordinals(prior)
    null_imports = discard_optional_null_imports([*objects, sdk], sdk_vars, retired)
    vacant = HOLES | set(retired.values())
    elf_symbols = symbol_types([elf], dynamic=True)
    interfaces = retained_interfaces(prior)
    required_interface_types(elf_symbols, sdk_vars, interfaces)
    exports = make_def(out / "xeno-port.def", prior, elf_symbols, sdk_vars, retired)
    for key, folder in (("ZIG_LOCAL_CACHE_DIR", "zig-local-cache"), ("ZIG_GLOBAL_CACHE_DIR", "zig-global-cache")):
        os.environ[key] = str(out / folder)
    flags, runtime = crt_options(args.zig, out / "crt-probe", BASE)
    image, release = out / "xeno-port.exe", out / "xeno-port-stripped.exe"
    map_file, lldmap = out / "xeno-port.map", out / "xeno-port.lldmap"
    cmd = [args.zig, "lld-link", *flags, "/debug:dwarf", "/fixed", "/dynamicbase:no", "/highentropyva:no",
           "/opt:noref", "/opt:noicf", "/timestamp:0", "/subsystem:console", "/map:" + str(map_file),
           "/lldmap:" + str(lldmap), "/def:" + str(out / "xeno-port.def"), "/out:" + str(image),
           *["/wrap:" + name for name in wraps], *map(str, objects + libs), *runtime]
    write_json(out / "xeno-port.link.json", cmd)
    with (out / "xeno-port.link.log").open("w") as stream:
        subprocess.run(cmd, cwd=out, stdout=stream, stderr=subprocess.STDOUT, check=True)
    full = audit_pe(image, exports, wraps, vacant)
    full_exports = pe_exports(image, holes=vacant)
    check_interface_owners(full_exports, image, lldmap, interfaces)
    headers = read(llvm_tool("readobj"), "--file-headers", "--sections", image)
    symbols = re.search(r"^  SymbolCount: (\d+)$", headers, re.M)
    require(symbols and int(symbols[1]) and {".debug_info",".debug_line"} <= set(re.findall(
            r"^    Name: (\.debug_\w+) ", headers, re.M)), "linked Windows image lacks COFF symbols or DWARF sections")
    compact = check_compact(image, lldmap, objects)
    bindings = verify_binding(image, lldmap, bridge, evidence, src, elf, required_callbacks=selected_callbacks())
    subprocess.run([llvm_tool("strip"), "--strip-all", "-o", release, image], check=True)
    stripped = audit_pe(release, exports, wraps, vacant)
    require(full_exports == pe_exports(release, holes=vacant), "stripping changed export addresses")
    binary_record = file_record(release)
    sdk_record = file_record(sdk)
    require(sdk_record["sha256"] == renderer_inputs["sdk_sha256"], "linked renderer archive changed during build")
    proof_path = out / "windows-renderer-inputs.json"
    write_json(proof_path, dict(renderer_inputs, binary_sha256=binary_record["sha256"]))
    receipt = {"schema": 1, "source": commit, "tree": tree, "platform": "windows", "mode": "full",
               "renderer_inputs": file_record(proof_path),
               "objects": fingerprint(objfile), "linux": fingerprint(linuxfile), "renderer": fingerprint(sdkfile),
               "ordinals_sha256": ABI_SHA256, "retired_sha256": RETIRED_SHA256, "exports": len(exports),
               "reserved_ordinals": sorted(vacant), "null_weak_imports": null_imports, "wrappers": wraps,
               "inputs": [fingerprint(p) for p in libs], "crt_inputs": [fingerprint(Path(p)) for p in runtime if Path(p).is_file()],
               "binary": binary_record, "sdk": sdk_record, "binding_evidence": file_record(evidence),
               "debug": fingerprint(image), "executable": file_record(release), "map": fingerprint(map_file),
               "lldmap": fingerprint(lldmap), "compact": compact, "binding": bindings, "debug_audit": full,
               "stripped_audit": stripped}
    write_json(out / "windows-link.json", receipt)
    print(f"Linked {release.name}: {receipt['executable']['sha256']}")


def self_check():
    from tempfile import TemporaryDirectory
    names = ordinals()
    require(names["EnterCriticalSection"] > 0, "missing public compatibility alias")
    retired = retired_ordinals(names)
    require(len(HOLES | set(retired.values())) == 3643 and len(names.keys() - retired.keys()) == 5397,
            "incorrect surviving or vacant slots")
    require(retired["PcPort_BattlingLoadFont"] == 2008 and retired["PcPort_BattlingPresentSelectionFrame"] == 8129
            and retired["PcPort_MovieIdct"] == 8222, "optional arena or movie ordinal changed")
    require(names["AmdPowerXpressRequestHighPerformance"] == 3 and names["NvOptimusEnablement"] == 1874
            and GPU_EXPORTS.isdisjoint(retired), "Windows GPU preference exports changed")
    require(all(retired.get(name) == ordinal for name, ordinal in {
        "PcPort_ArchiveFieldWdsActive": 1948, "PcPort_DiscLidOpen": 2116,
        "PcPort_QuickCheckpointHoldFieldPresentation": 8225, "PcPort_ToggleDiscLid": 2540,
    }.items()), "optional callback retirement ordinals changed")
    world_boundary = {
        "PcPort_WorldCaptureCurFrame", "PcPort_WorldCaptureFinish",
        "PcPort_WorldCaptureFrameComplete", "PcPort_WorldCaptureFulfillAtPresent",
        "PcPort_WorldCaptureLastFulfilledFrame", "PcPort_WorldCapturePending",
        "PcPort_WorldCaptureRequest", "PcPort_WorldCaptureReset",
        "PcPort_WorldCaptureSetFrame", "wm_80097800",
        "wm_sched_get_entry", "wm_sched_get_frontier_pc",
        "wm_sched_get_outcome", "wm_sched_reset",
        "GraphicsDrawPauseLetters", "SoundMuteAllSpuChannels",
        "func_8003101C", "GfxFreeWorkBuffers",
        "battle_func_800877E0",
    }
    require(world_boundary <= names.keys() - retired.keys() and
            retired.get("func_800877E0") == 4784 and retired.get("func_800879A8") == 4790,
            "World Map ordinal retirement overlaps the surviving engine and battle interfaces")
    movie_player_slots = {1078, 1091, 1092, 1785, 6028, 6031, 6036, 6038, 6040}
    require(retired.get("D_80076EEC") == 503 and retired.get("func_800769A4") == 4359 and
            movie_player_slots.isdisjoint(retired.values()), "movie-state and movie-player boundaries overlap")
    callbacks = selected_callbacks()
    require(callbacks.get("battle_func_80077698") == "battle_host_main7_port.c.o", "overlay-prefixed native entry lost")
    dropped_callbacks = {"battle_func_800742A0":3415, "func_800742A0":4319, "func_80076B00":4367,
                         "battle_func_800B5588":7994, "battle_func_800B56E4":7995,
                         "func_800B5588":7996, "func_800B56E4":7997}
    require(all(retired.get(name) == ordinal and name not in callbacks for name, ordinal in dropped_callbacks.items()),
            "retired callback ordinal reused")
    cells = ["field.o", "stubs.o"]
    rows = [{"name": n} for n in cells + [HOST_OBJECT]]
    require(ordered_objects({"schema":1,"mode":"full","objects":rows},cells) == rows, "valid input order rejected")
    def bad(test):
        try:
            test()
        except (ValueError, KeyError):
            return
        raise ValueError("unsupported input accepted")
    for mode, order in (("subset",rows),("full",rows[:-1]),("full",rows[::-1])):
        bad(lambda: ordered_objects({"schema":1,"mode":mode,"objects":order},cells))
    symbols = {"EnterCriticalSection":"T", "Word":"D"}
    internal = {name: "T" for name in LINK_ONLY}
    symbols.update(internal)
    win = {**internal,"XenoPsyQEnterCriticalSection":"T", "Word":"D", **{s:"D" for s in GPU_EXPORTS}}
    with TemporaryDirectory() as folder:
        path = Path(folder)/"test.def"
        baseline = {"Word":5}
        assigned = make_def(path, baseline, symbols, win, {})
        require(LINK_ONLY.isdisjoint(assigned) and assigned["Word"] == 5 and "Word @5 DATA" in path.read_text()
                and "EnterCriticalSection=XenoPsyQEnterCriticalSection" in path.read_text(), "bad export spelling, type or ordinal")
        bad(lambda: make_def(path, {"Lost":1}, symbols, win, {}))
        retired_path = Path(folder) / "retired.def"
        assigned = make_def(retired_path, {"Word":5, "Former":7}, symbols, win, {"Former":7})
        require(assigned["Word"] == 5 and "Former" not in assigned and min(v for k,v in assigned.items() if k != "Word") == 8,
                "retired ordinal reused or a surviving export moved")
        bad(lambda: make_def(path, {"Former":7}, dict(symbols,Former="T"),win,{"Former":7}))
        bad(lambda: make_def(path, {"Former":7}, symbols,dict(win,Former="T"),{"Former":7}))
        bad(lambda: make_def(path, {"Former":7}, symbols,win,{"Former":8}))
        interfaces = retained_interfaces(names)
        alive = {name: kind for name, (_, _, kind) in interfaces.items()}
        require(len(interfaces) == 89 and interfaces["MovieMain"] == (1784, "src_movie_main.c.o", "T") and
                interfaces["file1_bound_controller"][0] == 3796 and
                interfaces["PcPort_BattleSharedData"] == (1989, "battle_mips_runtime.c.o", "T"),
                "retained module or menu boundary changed")
        linux_types = {**alive, "PcPort_BattleSharedData": "W"}
        required_interface_types(linux_types, alive, interfaces)
        bad(lambda: required_interface_types(alive, alive, interfaces))
        bad(lambda: required_interface_types(linux_types, linux_types, interfaces))
        for name in ("PcPort_RoboMenuDispatch", "file1_bound_controller", "g_BattleResultsEntry", "PcPort_BattleSharedData", "MovieMain"):
            bad(lambda: required_interface_types(linux_types, {**alive, name: "R"}, interfaces))
        temp = Path(folder)
        sdk = temp / "renderer.a"
        member = temp / "demo.c.o"
        # Tiny COFF-header fixture exercises the archive and dependency join without an engine build.
        member.write_bytes(struct.pack("<HHIIIHH",0x8664,0,0,0,0,0,0))
        subprocess.run([llvm_tool("ar"),"rcs",sdk,member],check=True)
        primary = "pc_port/extern/PsyCross/src/demo.c"
        header = "pc_port/include_shim/demo.h"
        inputs = {primary:"1"*64,header:"2"*64, **dict.fromkeys(RENDERER_SOURCE_FILES, "e"*64)}
        project = {header:inputs[header], **dict.fromkeys(RENDERER_SOURCE_FILES.values(), "e"*64)}
        donor = {**inputs, **dict.fromkeys(RENDERER_SOURCE_FILES, "d"*64)}
        unit = {"member":member.name,"object_sha256":fingerprint(member)["sha256"],"compiler_inputs":list(inputs)}
        sdk_receipt = {"mode":"full","source_commit":"a"*40,"source_tree":"b"*40,"target":"x86_64-windows-gnu",
                       "compiler":"0.13.0","archive_sha256":fingerprint(sdk)["sha256"],"compiler_inputs":inputs,
                       "source_files":dict(RENDERER_SOURCE_FILES), "units":[unit]}
        profile = {"restored_inputs": donor, "sources": ["src/demo.c"], "target": sdk_receipt["target"],
                   "zig": sdk_receipt["compiler"], "unit_inputs": {member.name: [*inputs, "SDL/demo.h"]}}
        profile_bytes = json.dumps(profile).encode()
        sdk_receipt["profile"] = {"bytes": len(profile_bytes), "sha256": hashlib.sha256(profile_bytes).hexdigest()}
        def validate_sdk(record):
            return sdk_library(sdk, record, "a"*40, "b"*40, profile_bytes, project.__getitem__)
        result = validate_sdk(sdk_receipt)
        require(result["units"] == {member.name:list(inputs)} and result["member_sha256"] == {member.name:unit["object_sha256"]},
                "incomplete per-object renderer inputs")
        # Relabeling an unchanged subset as a full SDK must also fail if its supplied union is internally consistent.
        extra_profile = dict(profile, sources=["src/demo.c", "src/omitted.c"],
                             unit_inputs={**profile["unit_inputs"], "omitted.c.o": [primary,header]})
        extra_bytes = json.dumps(extra_profile).encode()
        extra_pin = {"bytes":len(extra_bytes), "sha256":hashlib.sha256(extra_bytes).hexdigest()}
        bad(lambda: sdk_library(sdk, dict(sdk_receipt, profile=extra_pin), "a"*40, "b"*40, extra_bytes,
                                project.__getitem__))
        for invalid in (dict(mode="subset"),dict(source_tree="c"*40),dict(archive_sha256="0"*64),
                        dict(profile={"bytes":len(profile_bytes),"sha256":"0"*64}), dict(compiler_inputs={**inputs,primary:"3"*64}),
                        dict(compiler_inputs={**inputs,header:"4"*64}), dict(units=[unit,unit]),
                        dict(compiler_inputs=donor), dict(source_files={}),
                        dict(units=[dict(unit,object_sha256="c"*64)]),
                        dict(units=[dict(unit,compiler_inputs=[primary])]),dict(units=[dict(unit,compiler_inputs=[header])]),
                        dict(units=[dict(unit,compiler_inputs=[primary,header,header])]),
                        dict(units=[dict(unit,compiler_inputs=[primary,"not-in-roster.h"])]),
                        dict(compiler_inputs={**inputs,"pc_port/include/../outside.h":"e"*64}),
                        dict(compiler_inputs={**inputs,"unknown/private.h":"e"*64})):
            bad(lambda: validate_sdk(dict(sdk_receipt,**invalid)))
        linux, win = temp / "linux", temp / "windows"
        for parent in (linux, win):
            parent.mkdir()
        elf, bridge = linux / "game", linux / "bridge"
        elf.write_bytes(b"\x7fELF\x02\x01" + bytes(10) + struct.pack("<HH",2,62))
        bridge.write_bytes(b"bridge table")
        generated = {"battle_bridge_map.inc": file_record(bridge)}
        linux_record = dict(source="a", tree="b", elf=file_record(elf), bridge=file_record(bridge), generated=generated)
        require(linux_inputs(linux_record, linux, "a", "b") == (elf, bridge), "rejected valid absolute Linux paths")
        rel_linux = dict(linux_record, elf=dict(linux_record["elf"], path=elf.name))
        require(linux_inputs(rel_linux, linux, "a", "b") == (elf, bridge), "rejected valid relative Linux paths")
        external = win / "elf-copy"
        external.write_bytes(elf.read_bytes())
        bad(lambda: linux_inputs(dict(linux_record,elf=file_record(external)), linux, "a","b"))
        link = linux / "elf-symlink"
        link.symlink_to(external)
        bad(lambda: linux_inputs(dict(linux_record,elf=dict(file_record(elf),path=link.name)), linux, "a","b"))
        # Windows object and generated pins remain relative to their compiler receipt.
        bad(lambda: local_artifact(win, file_record(external)))
        linuxfile = linux / "linux-link.json"
        linuxfile.write_text(json.dumps(linux_record))
        compilation = {"linux": file_record(linuxfile)}
        verify_linux_pin(compilation, win, linuxfile)
        bad(lambda: verify_linux_pin(dict(compilation,linux=dict(compilation["linux"],sha256="0"*64)),win,linuxfile))
        copied = linux / "copied.json"
        copied.write_bytes(linuxfile.read_bytes())
        bad(lambda: verify_linux_pin(compilation, win, copied))
        wt = win / "gen"
        wt.mkdir()
        evidence = wt / "battle_win32_binding_evidence.json"
        table = wt / "battle_win32_binding_sizes.inc"
        wb = wt / "battle_bridge_map.inc"
        evidence.write_text("{}")
        table.write_text("sizes")
        wb.write_bytes(bridge.read_bytes())
        recs = {p.name:dict(file_record(p),path=str(p.relative_to(win))) for p in (evidence,table,wb)}
        def consumer(owner, p):
            return {"name":owner,"dependencies":{"generated/"+p.relative_to(win).as_posix():recs[p.name]["sha256"]}}
        compilation.update(generated=recs,objects=[consumer(HOST_OBJECT,table),consumer("battle_mips_runtime.c.o",wb)])
        require(generated_bindings(compilation,win,wt,bridge)==evidence,"generated evidence not bound")
        bad(lambda: generated_bindings(compilation,win,linux,bridge))
        bad(lambda: generated_bindings(dict(compilation,objects=[{"name":HOST_OBJECT,"dependencies":{}},
                    compilation["objects"][1]]),win,wt,bridge))
        alternate_bridge = linux / "other-bridge"
        alternate_bridge.write_bytes(b"another logical bridge")
        bad(lambda: generated_bindings(compilation,win,wt,alternate_bridge))
        evidence.write_text('{"changed":true}')
        bad(lambda: generated_bindings(compilation,win,wt,bridge))
    print("Public ABI, full roster, producer path/hash boundaries and compiled bridge dependency checks passed.")


if __name__ == "__main__":
    if sys.argv[1:] == ["--self-check"]:
        self_check()
    else:
        parser = argparse.ArgumentParser(description=__doc__)
        for key in ("objects", "linux", "sdk", "sdk-receipt", "prefix", "generated", "source-root", "out"):
            parser.add_argument("--"+key, required=True, type=Path)
        parser.add_argument("--zig", default="zig")
        link(parser.parse_args())
