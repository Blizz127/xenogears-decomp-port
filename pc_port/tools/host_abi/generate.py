#!/usr/bin/env python3
"""Generate Xenogears US host bridge and placeholder declarations from named ABI facts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import tempfile


HERE = Path(__file__).resolve().parent
PROFILE = HERE / "host-abi.json"
TEMPLATE = HERE / "stubs.c.in"
PROFILE_SHA256 = "45722ee154f65dfce893bb620175db58a0577f02abc5b1d084720d28dc18bb0b"
TEMPLATE_SHA256 = "c3238ce13e6625d6fa30a0b31cc55885cb6d6f6a49d8497cc6c87d7b0602a054"
ASSIGNMENT = re.compile(
    r"^\s*([A-Za-z_.$][\w.$]*)\s*=\s*(0x[0-9a-fA-F]+)\s*;"
    r"(?:.*?\bsize:\s*(0x[0-9a-fA-F]+|\d+))?", re.ASCII)
HELPERS = {"xeno_port_stub", "xeno_port_is_generated_stub"}
KNOWN_CALLS = {"bzero", "memcpy", "memmove", "memset", "printf", "rand", "ratan2", "rcos", "rsin"}


def checked_text(path, expected):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != expected:
        raise ValueError(f"Unexpected SHA-256 for {path.name}")
    return raw.decode("utf-8")


def bridge(args, profile):
    functions = set(profile["main_function_names"])
    names = {}
    for key, digest in profile["input_sha256"].items():
        path = getattr(args, key)
        for line in checked_text(path, digest).splitlines():
            match = ASSIGNMENT.match(line)
            if not match:
                continue
            name, raw_address, explicit_size = match.groups()
            if name.startswith((".L", "L8")):
                continue
            address = int(raw_address, 16)
            size = int(explicit_size, 0) if explicit_size else 0
            is_function = name in functions or name in KNOWN_CALLS or name.startswith(("func_", "Heap"))
            if name in names:
                other_address, old_size, _ = names[name]
                if address != other_address:
                    raise ValueError(f"Ambiguous address for {name}")
                size = max(old_size, size)
            names[name] = (address, size, is_function)
    if not functions.issubset(names):
        raise ValueError("The supplied symbol inputs lack a required function classification")
    if args.phase == "settled":
        for name, size in profile["settled_clamped_sizes"].items():
            address, _, is_function = names[name]
            if is_function:
                raise ValueError(f"Cannot apply a storage extent to {name}")
            names[name] = (address, size, False)
        for name, size in profile["settled_split_sizes"].items():
            if name in names or not re.fullmatch(r"D_[0-9A-F]{8}", name):
                raise ValueError(f"Invalid split object {name}")
            names[name] = (int(name[2:], 16), size, False)

    ordered = sorted(names, key=lambda name: (names[name][0], not names[name][2], name))
    width = max(len(name.encode("ascii")) for name in names) + 1
    lines = [
        "/* Generated from retail symbol maps; do not edit. */", "typedef struct {",
        "    uint32_t address, size;", "    uint8_t is_function;", f"    char name[{width}];",
        "} PcPortInlineBattleSymbol;",
        "static const PcPortInlineBattleSymbol g_BattleBridgeSymbols[] HOST_ONLY_RODATA = {",
    ]
    for name in ordered:
        address, size, is_function = names[name]
        lines.append(f'    {{ 0x{address:08x}u, {size}u, {int(is_function)}, "{name}" }},')
    lines.extend(["};", "static const size_t g_BattleBridgeSymbolCount = "
                  "sizeof(g_BattleBridgeSymbols) / sizeof(g_BattleBridgeSymbols[0]);"])
    return "\n".join(lines) + "\n"


def stubs(args, profile):
    expected = profile["native_object_sizes"]
    observed = set(args.undefined.read_text(encoding="utf-8").split()) - HELPERS
    if observed - set(expected) or (not args.permit_unused_storage and observed != set(expected)):
        missing = sorted(set(expected) - observed)
        unknown = sorted(observed - set(expected))
        raise ValueError(f"Native link and type profile disagree (missing {missing!r}, unknown {unknown!r})")
    lines = []
    for name, size in sorted(expected.items()):
        section = ', section(".data.compact_gpu")' if name == "g_FieldRenderContexts" else ""
        lines.append(f"unsigned char {name}[{size}] __attribute__((aligned(8){section}));")
    template = checked_text(TEMPLATE, TEMPLATE_SHA256)
    marker = "@NATIVE_OBJECT_DECLARATIONS@"
    if template.count(marker) != 1:
        raise ValueError("Invalid native declaration template")
    return template.replace(marker, "\n".join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subcommands = parser.add_subparsers(dest="command", required=True)
    b = subcommands.add_parser("bridge", help="Generate the guest-address to native-symbol bridge include")
    b.add_argument("--phase", choices=["cold", "settled"], required=True)
    for key in ("main-symbols", "battle-functions", "battle-symbols"):
        b.add_argument("--" + key, type=Path, required=True)
    s = subcommands.add_parser("stubs", help="Generate native storage for this host link")
    s.add_argument("--permit-unused-storage",action="store_true",
                   help="Keep the known native storage ABI when optional callers are omitted")
    s.add_argument("--undefined", type=Path, required=True, help="One missing symbol name per line")
    for command in (s, b):
        command.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    profile = json.loads(checked_text(PROFILE, PROFILE_SHA256))
    inputs = {PROFILE.resolve(), TEMPLATE.resolve(), Path(__file__).resolve()}
    inputs.update(value.resolve() for name, value in vars(args).items()
                  if isinstance(value, Path) and name != "out")
    if args.out.resolve() in inputs or args.out.is_symlink():
        raise ValueError("Choose a distinct regular output file")
    data = (bridge(args, profile) if args.command == "bridge" else stubs(args, profile)).encode("utf-8")
    args.out.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=args.out.parent, delete=False) as out:
        staged = Path(out.name)
        try:
            out.write(data)
            out.flush()
        except OSError:
            out.close()
            staged.unlink(missing_ok=True)
            raise
    try:
        os.replace(staged, args.out)
    finally:
        staged.unlink(missing_ok=True)


if __name__ == "__main__":
    try:
        main()
    except (KeyError, OSError, ValueError) as error:
        sys.exit(str(error))
