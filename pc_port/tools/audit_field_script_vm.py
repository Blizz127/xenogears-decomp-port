#!/usr/bin/env python3
"""Evidence-oriented audit of the two retail field-script dispatch tables.

This tool deliberately keeps three different questions separate:

* Does the port dispatch each opcode to the same symbol/address as retail?
* Does the current PSYQ object reproduce the retail instruction stream?
* Does the native port actually provide retail behavior, rather than a
  generated or hand-authored no-op stub?

The first question can be answered from a clean checkout plus ``disc/field.bin``.
The optional matching-object comparison needs a freshly built decomp checkout
passed with ``--matching-root``.  Native call edges are intentionally direct
only; a path-insensitive transitive closure turns error/debug paths into false
claims about ordinary script execution.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import struct
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, Sequence


EXPECTED_TABLE_LENGTHS = {
    "g_FieldScriptVMHandlers": 0x100,
    "g_FieldScriptVMHandlers2": 0xE3,
}
FIELD_RETAIL_SHA256 = (
    "38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"
)

TABLE_MARKERS = {
    "g_FieldScriptVMHandlers": (
        '"g_FieldScriptVMHandlers:\\n");',
        'asm(".balign 8\\n"',
    ),
    "g_FieldScriptVMHandlers2": (
        '"g_FieldScriptVMHandlers2:\\n");',
        None,
    ),
}

MIPS_RELOC_MASKS = {
    "R_MIPS_NONE": 0xFFFFFFFF,
    "R_MIPS_16": 0xFFFF0000,
    "R_MIPS_32": 0x00000000,
    "R_MIPS_REL32": 0x00000000,
    "R_MIPS_26": 0xFC000000,
    "R_MIPS_HI16": 0xFFFF0000,
    "R_MIPS_LO16": 0xFFFF0000,
    "R_MIPS_GPREL16": 0xFFFF0000,
    "R_MIPS_LITERAL": 0xFFFF0000,
    "R_MIPS_GOT16": 0xFFFF0000,
    "R_MIPS_PC16": 0xFFFF0000,
    "R_MIPS_CALL16": 0xFFFF0000,
    "R_MIPS_GPREL32": 0x00000000,
}

INCOMPLETE_MARKERS = re.compile(
    r"\b(?:DEFERRED|UNPORTED|UNIMPLEMENTED|NOT IMPLEMENTED|"
    r"PLACEHOLDER|SAFE[- ]RETURN|OMITTED)\b",
    re.IGNORECASE,
)
NATIVE_MACROS = {
    "XENO_PC_PORT",
    "XENO_FIELD_OBJECT_OVERLAY",
    "XENO_NATIVE",
}

STRICT_RUNTIME_BLOCKING_CLASSES = {
    "INCOMPLETE_NATIVE_STUB",
    "INCOMPLETE_AUTHORED_STUB",
    "INCOMPLETE_DIRECT_DEPENDENCY",
    "INCOMPLETE_AUTHORED_DEPENDENCY",
    "INCOMPLETE_SOURCE_MARKER",
    "MISSING_SOURCE",
}


class AuditError(RuntimeError):
    pass


@dataclass(frozen=True)
class Symbol:
    obj: Path
    address: int
    size: int


@dataclass(frozen=True)
class Relocation:
    kind: str
    target: str


@dataclass
class FunctionSource:
    path: str = ""
    line: int = 0
    kind: str = "missing"
    conditionals: list[str] | None = None
    markers: list[str] | None = None


@dataclass
class HandlerRow:
    handler: str
    retail_address: str
    slots: list[str]
    source_path: str
    source_line: int
    source_kind: str
    source_conditionals: list[str]
    source_markers: list[str]
    native_generated_stub: bool
    native_authored_stub: bool
    direct_generated_stub_calls: list[str]
    direct_authored_stub_calls: list[str]
    mips_comparison: str
    retail_size: int | None
    current_size: int | None
    relocation_checks: int
    audit_class: str
    detail: str


def run_checked(args: Sequence[str], cwd: Path | None = None) -> str:
    try:
        completed = subprocess.run(
            list(args),
            cwd=cwd,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except FileNotFoundError as exc:
        raise AuditError(f"required tool not found: {args[0]}") from exc
    except subprocess.CalledProcessError as exc:
        raise AuditError(
            f"command failed ({exc.returncode}): {' '.join(args)}\n{exc.stderr.strip()}"
        ) from exc
    return completed.stdout


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_symbol_addresses(path: Path) -> dict[str, int]:
    result: dict[str, int] = {}
    expression = re.compile(r"^\s*([A-Za-z_$][\w.$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;")
    for line in path.read_text(encoding="utf-8").splitlines():
        match = expression.match(line)
        if match:
            result[match.group(1)] = int(match.group(2), 16)
    return result


def resolve_handler_address(name: str, symbols: dict[str, int]) -> int | None:
    """Resolve both named symbols and canonical func_XXXXXXXX labels."""

    if name in symbols:
        return symbols[name]
    match = re.fullmatch(r"func_([0-9A-Fa-f]{8})", name)
    if match:
        return int(match.group(1), 16)
    return None


def parse_field_vram(path: Path) -> int:
    text = path.read_text(encoding="utf-8")
    match = re.search(
        r"(?ms)^segments:\s*\n\s*-\s+name:\s*field\b.*?^\s+vram:\s*(0x[0-9A-Fa-f]+)",
        text,
    )
    if not match:
        raise AuditError(f"cannot resolve field segment VRAM from {path}")
    return int(match.group(1), 16)


def parse_port_tables(path: Path) -> dict[str, list[str]]:
    text = path.read_text(encoding="utf-8")
    result: dict[str, list[str]] = {}
    handler_re = re.compile(r"^FIELD_VM_HANDLER\(([^)]+)\)\s*$", re.MULTILINE)
    for table, (start_marker, end_marker) in TABLE_MARKERS.items():
        try:
            start = text.index(start_marker) + len(start_marker)
        except ValueError as exc:
            raise AuditError(f"missing {table} marker in {path}") from exc
        if end_marker is None:
            end = len(text)
        else:
            try:
                end = text.index(end_marker, start)
            except ValueError as exc:
                raise AuditError(f"missing end marker for {table} in {path}") from exc
        result[table] = [item.strip() for item in handler_re.findall(text[start:end])]
    return result


def read_retail_tables(
    binary: Path,
    field_vram: int,
    symbols: dict[str, int],
) -> dict[str, list[int]]:
    payload = binary.read_bytes()
    result: dict[str, list[int]] = {}
    for table, count in EXPECTED_TABLE_LENGTHS.items():
        if table not in symbols:
            raise AuditError(f"missing address for {table}")
        offset = symbols[table] - field_vram
        end = offset + count * 4
        if offset < 0 or end > len(payload):
            raise AuditError(
                f"{table} range {offset:#x}..{end:#x} is outside {binary}"
            )
        result[table] = list(struct.unpack(f"<{count}I", payload[offset:end]))
    return result


def slot_label(table: str, index: int) -> str:
    if table == "g_FieldScriptVMHandlers":
        return f"{index:02X}"
    return f"FE{index:02X}"


def strip_c_for_braces(text: str) -> str:
    """Replace comments/strings with spaces while preserving offsets/newlines."""

    output = list(text)
    i = 0
    state = "code"
    quote = ""
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if ch == "/" and nxt == "/":
                output[i] = output[i + 1] = " "
                state = "line_comment"
                i += 2
                continue
            if ch == "/" and nxt == "*":
                output[i] = output[i + 1] = " "
                state = "block_comment"
                i += 2
                continue
            if ch in {'"', "'"}:
                quote = ch
                output[i] = " "
                state = "string"
        elif state == "line_comment":
            if ch == "\n":
                state = "code"
            else:
                output[i] = " "
        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                output[i] = output[i + 1] = " "
                state = "code"
                i += 2
                continue
            if ch != "\n":
                output[i] = " "
        elif state == "string":
            if ch == "\\":
                output[i] = " "
                if i + 1 < len(text):
                    if text[i + 1] != "\n":
                        output[i + 1] = " "
                    i += 2
                    continue
            output[i] = "\n" if ch == "\n" else " "
            if ch == quote:
                state = "code"
        i += 1
    return "".join(output)


def preprocessor_stack_by_line(text: str) -> dict[int, list[str]]:
    stacks: dict[int, list[str]] = {}
    stack: list[str] = []
    for number, line in enumerate(text.splitlines(), 1):
        stacks[number] = list(stack)
        directive = re.match(r"\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)", line)
        if not directive:
            continue
        command, operand = directive.groups()
        operand = operand.strip()
        if command in {"ifdef", "ifndef", "if"}:
            stack.append(f"{command} {operand}".strip())
        elif command == "elif" and stack:
            stack[-1] = f"elif {operand}".strip()
        elif command == "else" and stack:
            stack[-1] = f"else({stack[-1]})"
        elif command == "endif" and stack:
            stack.pop()
    return stacks


def find_matching_brace(cleaned: str, opening: int) -> int:
    depth = 0
    for position in range(opening, len(cleaned)):
        if cleaned[position] == "{":
            depth += 1
        elif cleaned[position] == "}":
            depth -= 1
            if depth == 0:
                return position + 1
    return len(cleaned)


def native_condition_may_compile(conditionals: Iterable[str]) -> bool:
    """Conservatively evaluate simple XENO_PC_PORT exclusion branches.

    Unknown preprocessor expressions remain eligible.  The only safe rows to
    remove from the native frontier are branches that explicitly select the
    non-port side.
    """

    false_when_port = {
        "ifndef XENO_PC_PORT",
        "if !defined(XENO_PC_PORT)",
        "if !defined XENO_PC_PORT",
        "elif !defined(XENO_PC_PORT)",
        "elif !defined XENO_PC_PORT",
        "else(ifdef XENO_PC_PORT)",
        "else(if defined(XENO_PC_PORT))",
        "else(if defined XENO_PC_PORT)",
    }
    return not any(" ".join(item.split()) in false_when_port for item in conditionals)


def scan_field_sources(root: Path, handlers: set[str]) -> dict[str, FunctionSource]:
    definitions: dict[str, list[FunctionSource]] = defaultdict(list)
    asm_sites: dict[str, list[FunctionSource]] = defaultdict(list)
    definition_re = re.compile(
        r"(?m)^[ \t]*(?:(?:static|inline|extern)\s+)*"
        r"(?:[A-Za-z_]\w*\s+)+(?:\*\s*)?([A-Za-z_]\w*)\s*"
        r"\([^;{}]*\)\s*\{"
    )
    asm_re = re.compile(r"INCLUDE_ASM\([^,]+,\s*([A-Za-z_]\w*)\s*\)\s*;")

    for path in sorted((root / "src/field").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="replace")
        cleaned = strip_c_for_braces(text)
        stacks = preprocessor_stack_by_line(text)
        relative = str(path.relative_to(root))

        for match in asm_re.finditer(text):
            name = match.group(1)
            if name not in handlers:
                continue
            line = text.count("\n", 0, match.start()) + 1
            asm_sites[name].append(
                FunctionSource(
                    path=relative,
                    line=line,
                    kind="include_asm",
                    conditionals=stacks.get(line, []),
                    markers=[],
                )
            )

        for match in definition_re.finditer(cleaned):
            name = match.group(1)
            if name not in handlers:
                continue
            opening = cleaned.find("{", match.start(), match.end())
            end = find_matching_brace(cleaned, opening)
            line = text.count("\n", 0, match.start()) + 1
            body = text[opening:end]
            markers = sorted({m.group(0).upper() for m in INCOMPLETE_MARKERS.finditer(body)})
            nested_conditionals = re.findall(
                r"(?m)^\s*#\s*(?:if|ifdef|ifndef)\b([^\n]*)", body
            )
            definitions[name].append(
                FunctionSource(
                    path=relative,
                    line=line,
                    kind="c",
                    conditionals=sorted(
                        set(stacks.get(line, []) + [item.strip() for item in nested_conditionals])
                    ),
                    markers=markers,
                )
            )

    result: dict[str, FunctionSource] = {}
    for handler in handlers:
        c_sites = definitions.get(handler, [])
        a_sites = asm_sites.get(handler, [])
        if c_sites:
            chosen = c_sites[0]
            if a_sites:
                chosen.kind = "native_c_override"
                chosen.conditionals = sorted(
                    set((chosen.conditionals or []) + (a_sites[0].conditionals or []))
                )
            result[handler] = chosen
        elif a_sites:
            result[handler] = a_sites[0]
        else:
            result[handler] = FunctionSource(conditionals=[], markers=[])
    return result


def parse_generated_stubs(path: Path) -> set[str]:
    if not path.is_file():
        return set()
    text = path.read_text(encoding="utf-8", errors="replace")
    return set(
        re.findall(
            r"(?m)^\s*(?:long|int|void)\s+([A-Za-z_$][\w.$]*)\s*"
            r"\([^)]*\)\s*\{[^{}]*\bxeno_port_stub\s*\(",
            text,
        )
    )


def parse_generated_data(path: Path) -> dict[str, int]:
    if not path.is_file():
        return {}
    text = path.read_text(encoding="utf-8", errors="replace")
    return {
        name: int(size)
        for name, size in re.findall(
            r"(?m)^\s*unsigned\s+char\s+([A-Za-z_$][\w.$]*)\s*"
            r"\[(\d+)\]\s+__attribute__",
            text,
        )
    }


def scan_authored_stub_functions(root: Path) -> list[dict]:
    """Find checked-in native function bodies that call xeno_port_stub.

    Generated ``build_native/stubs.c`` is audited separately. This scanner
    covers source-authored placeholders, while ignoring comments and string
    literals so diagnostic prose cannot create a false blocker.
    """

    source_roots = [root / "pc_port/src", root / "src"]
    source_roots = [path for path in source_roots if path.is_dir()]
    if not source_roots:
        return []
    definition_re = re.compile(
        r"(?m)^[ \t]*(?:(?:static|inline|extern)\s+)*"
        r"(?:[A-Za-z_]\w*\s+)+(?:\*\s*)?([A-Za-z_]\w*)\s*"
        r"\([^;{}]*\)\s*\{"
    )
    call_re = re.compile(r"\bxeno_port_stub\s*\(")
    rows: list[dict] = []
    for source_root in source_roots:
        for path in sorted(source_root.rglob("*.c")):
            text = path.read_text(encoding="utf-8", errors="replace")
            cleaned = strip_c_for_braces(text)
            relative = str(path.relative_to(root))
            for definition in definition_re.finditer(cleaned):
                opening = cleaned.find("{", definition.start(), definition.end())
                ending = find_matching_brace(cleaned, opening)
                calls = list(call_re.finditer(cleaned, opening, ending))
                if not calls:
                    continue
                rows.append(
                    {
                        "function": definition.group(1),
                        "path": relative,
                        "line": text.count("\n", 0, definition.start()) + 1,
                        "stub_call_lines": [
                            text.count("\n", 0, call.start()) + 1 for call in calls
                        ],
                    }
                )
    return rows


def collect_object_symbols(tool_prefix: str, objects: Iterable[Path]) -> dict[str, list[Symbol]]:
    index: dict[str, list[Symbol]] = defaultdict(list)
    nm = f"{tool_prefix}nm" if tool_prefix else "nm"
    expression = re.compile(
        r"^([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+([TtWw])\s+([^\s]+)$"
    )
    for obj in objects:
        output = run_checked([nm, "-S", "--defined-only", str(obj)])
        for line in output.splitlines():
            match = expression.match(line.strip())
            if match:
                address, size, _kind, name = match.groups()
                index[name].append(Symbol(obj=obj, address=int(address, 16), size=int(size, 16)))
    return index


def parse_mips_object(path: Path) -> tuple[dict[int, int], dict[int, list[Relocation]]]:
    output = run_checked(["mips-linux-gnu-objdump", "-drz", str(path)])
    words: dict[int, int] = {}
    relocs: dict[int, list[Relocation]] = defaultdict(list)
    instruction_re = re.compile(r"^\s*([0-9A-Fa-f]+):\s+([0-9A-Fa-f]{8})\s")
    relocation_re = re.compile(
        r"^\s*([0-9A-Fa-f]+):\s+(R_MIPS_[A-Za-z0-9_]+)\s+(.+?)\s*$"
    )
    for line in output.splitlines():
        instruction = instruction_re.match(line)
        if instruction:
            words[int(instruction.group(1), 16)] = int(instruction.group(2), 16)
            continue
        relocation = relocation_re.match(line)
        if relocation:
            address, kind, target = relocation.groups()
            relocs[int(address, 16)].append(Relocation(kind=kind, target=target.strip()))
    return words, relocs


def normalized_reloc_target(target: str) -> str:
    target = re.sub(r"[+-]0x[0-9A-Fa-f]+$", "", target.strip())
    return target


def is_section_relocation(target: str) -> bool:
    target = normalized_reloc_target(target)
    return target.startswith(".") or target.startswith("$")


def external_relocation_signature(
    relocations: Iterable[Relocation], instruction: int = 0
) -> Counter:
    """Return kind, target, and encoded addend for external relocations."""

    return Counter(
        (
            relocation.kind,
            normalized_reloc_target(relocation.target),
            instruction
            & (~MIPS_RELOC_MASKS.get(relocation.kind, 0xFFFFFFFF) & 0xFFFFFFFF),
        )
        for relocation in relocations
        if not is_section_relocation(relocation.target)
    )


def section_relocation_signature(
    relocations: Iterable[Relocation], instruction: int, owner: Symbol
) -> Counter:
    """Normalize section-local relocations without discarding their addends.

    Matching compiler output commonly emits an R_MIPS_26 against ``.text``
    for a jump within the current function. Object layout can move the whole
    function, so compare that target relative to the function start. All other
    section references retain both their section name and encoded relocation
    operand, which fails conservatively if data/call ownership is ambiguous.
    """

    signature: Counter = Counter()
    for relocation in relocations:
        if not is_section_relocation(relocation.target):
            continue
        target = normalized_reloc_target(relocation.target)
        if relocation.kind == "R_MIPS_26" and target == ".text":
            section_offset = (instruction & 0x03FFFFFF) << 2
            if owner.address <= section_offset < owner.address + owner.size:
                normalized = ("SELF", section_offset - owner.address)
            else:
                normalized = (target, section_offset)
        else:
            mask = MIPS_RELOC_MASKS.get(relocation.kind)
            encoded_operand = instruction if mask is None else instruction & ~mask
            normalized = (target, encoded_operand)
        signature[(relocation.kind, normalized)] += 1
    return signature


def compare_mips_symbols(
    current: Symbol,
    retail: Symbol,
    cache: dict[Path, tuple[dict[int, int], dict[int, list[Relocation]]]],
) -> tuple[str, int, str]:
    if current.size != retail.size:
        return "size_mismatch", 0, f"current={current.size} retail={retail.size}"
    if current.size % 4:
        return "invalid_size", 0, f"non-word function size {current.size}"

    if current.obj not in cache:
        cache[current.obj] = parse_mips_object(current.obj)
    if retail.obj not in cache:
        cache[retail.obj] = parse_mips_object(retail.obj)
    current_words, current_relocs = cache[current.obj]
    retail_words, retail_relocs = cache[retail.obj]
    relocation_checks = 0

    for relative in range(0, current.size, 4):
        current_address = current.address + relative
        retail_address = retail.address + relative
        if current_address not in current_words or retail_address not in retail_words:
            return "missing_instruction", relocation_checks, f"relative={relative:#x}"

        current_at = current_relocs.get(current_address, [])
        retail_at = retail_relocs.get(retail_address, [])
        mask = 0xFFFFFFFF
        for relocation in current_at + retail_at:
            if relocation.kind not in MIPS_RELOC_MASKS:
                return (
                    "unknown_relocation",
                    relocation_checks,
                    f"relative={relative:#x} kind={relocation.kind}",
                )
            mask &= MIPS_RELOC_MASKS[relocation.kind]

        if (current_words[current_address] & mask) != (retail_words[retail_address] & mask):
            return (
                "code_mismatch",
                relocation_checks,
                f"relative={relative:#x} current={current_words[current_address]:08x} "
                f"retail={retail_words[retail_address]:08x} mask={mask:08x}",
            )

        current_targets = external_relocation_signature(
            current_at, current_words[current_address]
        )
        retail_targets = external_relocation_signature(
            retail_at, retail_words[retail_address]
        )
        current_sections = section_relocation_signature(
            current_at, current_words[current_address], current
        )
        retail_sections = section_relocation_signature(
            retail_at, retail_words[retail_address], retail
        )
        relocation_checks += sum(current_targets.values()) + sum(
            current_sections.values()
        )
        if current_targets != retail_targets:
            return (
                "relocation_target_mismatch",
                relocation_checks,
                f"relative={relative:#x} current={dict(current_targets)} "
                f"retail={dict(retail_targets)}",
            )
        if current_sections != retail_sections:
            return (
                "relocation_target_mismatch",
                relocation_checks,
                f"relative={relative:#x} current_section={dict(current_sections)} "
                f"retail_section={dict(retail_sections)}",
            )
    return "match", relocation_checks, ""


def locate_matching_objects(root: Path) -> tuple[dict[str, list[Symbol]], dict[str, list[Symbol]]]:
    current_objects = sorted((root / "build/src/field").rglob("*.c.o"))
    retail_objects = [
        path
        for path in sorted((root / "build/asm/field").rglob("*.s.o"))
        if "/data/" not in path.as_posix()
    ]
    if not current_objects or not retail_objects:
        raise AuditError(
            f"{root} lacks fresh field compiler/full-ASM objects; build build/out/field.elf first"
        )
    return (
        collect_object_symbols("mips-linux-gnu-", current_objects),
        collect_object_symbols("mips-linux-gnu-", retail_objects),
    )


def parse_native_relocations(path: Path) -> dict[int, list[str]]:
    output = run_checked(["objdump", "-dr", str(path)])
    result: dict[int, list[str]] = defaultdict(list)
    expression = re.compile(r"^\s*([0-9A-Fa-f]+):\s+R_[A-Za-z0-9_]+\s+(.+?)\s*$")
    for line in output.splitlines():
        match = expression.match(line)
        if not match:
            continue
        address = int(match.group(1), 16)
        target = re.sub(r"[+-]0x[0-9A-Fa-f]+$", "", match.group(2).strip())
        result[address].append(target)
    return result


def direct_native_stub_edges(
    root: Path,
    handlers: set[str],
    stubs: set[str],
) -> dict[str, list[str]]:
    object_root = root / "pc_port/build_native/obj"
    if not object_root.is_dir() or not stubs:
        return {}
    objects = [
        path
        for path in sorted(object_root.rglob("*.o"))
        if path.name != "stubs.o"
    ]
    if not objects:
        return {}
    symbol_index = collect_object_symbols("", objects)
    reloc_cache: dict[Path, dict[int, list[str]]] = {}
    result: dict[str, list[str]] = {}
    for handler in sorted(handlers):
        candidates = symbol_index.get(handler, [])
        if not candidates:
            continue
        symbol = candidates[0]
        if symbol.obj not in reloc_cache:
            reloc_cache[symbol.obj] = parse_native_relocations(symbol.obj)
        targets: set[str] = set()
        for address, names in reloc_cache[symbol.obj].items():
            if symbol.address <= address < symbol.address + symbol.size:
                targets.update(name for name in names if name in stubs)
        if targets:
            result[handler] = sorted(targets)
    return result


def field_native_generated_edges(root: Path, generated: set[str]) -> list[dict]:
    """List direct generated-symbol references from native field functions.

    This is a conservative static inventory, not a claim that every caller is
    reachable in a particular retail script or runtime trace.
    """

    object_root = root / "pc_port/build_native/obj"
    if not object_root.is_dir() or not generated:
        return []
    objects = sorted(object_root.glob("src_field*.o"))
    overlay = object_root / "field_object_overlay.c.o"
    if overlay.is_file():
        objects.append(overlay)
    if not objects:
        return []

    symbol_index = collect_object_symbols("", objects)
    reloc_cache: dict[Path, dict[int, list[str]]] = {}
    rows: list[dict] = []
    for caller, symbols in symbol_index.items():
        for symbol in symbols:
            if symbol.obj not in reloc_cache:
                reloc_cache[symbol.obj] = parse_native_relocations(symbol.obj)
            targets: set[str] = set()
            for address, names in reloc_cache[symbol.obj].items():
                if symbol.address <= address < symbol.address + symbol.size:
                    targets.update(name for name in names if name in generated)
            if targets:
                rows.append(
                    {
                        "caller": caller,
                        "object": symbol.obj.name,
                        "generated_stub_targets": sorted(targets),
                    }
                )
    return sorted(rows, key=lambda row: (row["object"], row["caller"]))


def filter_generated_edges(rows: Iterable[dict], symbols: set[str]) -> list[dict]:
    result: list[dict] = []
    for row in rows:
        targets = [
            target for target in row["generated_stub_targets"] if target in symbols
        ]
        if targets:
            result.append({**row, "generated_stub_targets": targets})
    return result


def scan_active_assert_zero(root: Path) -> list[dict]:
    """Find executable assert(0...) sites inside field C function bodies."""

    definition_re = re.compile(
        r"(?m)^[ \t]*(?:(?:static|inline|extern)\s+)*"
        r"(?:[A-Za-z_]\w*\s+)+(?:\*\s*)?([A-Za-z_]\w*)\s*"
        r"\([^;{}]*\)\s*\{"
    )
    assert_re = re.compile(r"\bassert\s*\(\s*0\b")
    rows: list[dict] = []
    for path in sorted((root / "src/field").rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="replace")
        cleaned = strip_c_for_braces(text)
        stacks = preprocessor_stack_by_line(text)
        relative = str(path.relative_to(root))
        for definition in definition_re.finditer(cleaned):
            opening = cleaned.find("{", definition.start(), definition.end())
            ending = find_matching_brace(cleaned, opening)
            for match in assert_re.finditer(cleaned, opening, ending):
                line = text.count("\n", 0, match.start()) + 1
                rows.append(
                    {
                        "function": definition.group(1),
                        "path": relative,
                        "line": line,
                        "conditionals": stacks.get(line, []),
                        "native_may_compile": native_condition_may_compile(
                            stacks.get(line, [])
                        ),
                    }
                )
    return rows


def choose_unique_symbol(index: dict[str, list[Symbol]], name: str, label: str) -> Symbol | None:
    values = index.get(name, [])
    if not values:
        return None
    if len(values) > 1:
        paths = ", ".join(str(value.obj) for value in values)
        raise AuditError(f"duplicate {label} symbol {name}: {paths}")
    return values[0]


def classify_row(
    source: FunctionSource,
    native_stub: bool,
    native_authored_stub: bool,
    direct_stubs: list[str],
    direct_authored_stubs: list[str],
    comparison: str,
) -> tuple[str, str]:
    if native_stub:
        return "INCOMPLETE_NATIVE_STUB", "native link supplies generated no-op"
    if native_authored_stub:
        return "INCOMPLETE_AUTHORED_STUB", "checked-in native body calls xeno_port_stub"
    if source.markers:
        return (
            "INCOMPLETE_SOURCE_MARKER",
            "source context declares incomplete work: " + ", ".join(source.markers),
        )
    if direct_stubs:
        return (
            "INCOMPLETE_DIRECT_DEPENDENCY",
            "direct call(s) resolve to generated no-op: " + ", ".join(direct_stubs),
        )
    if direct_authored_stubs:
        return (
            "INCOMPLETE_AUTHORED_DEPENDENCY",
            "direct call(s) resolve to authored no-op: "
            + ", ".join(direct_authored_stubs),
        )
    conditional_text = " ".join(source.conditionals or [])
    if source.kind == "native_c_override" or any(
        macro in conditional_text for macro in NATIVE_MACROS
    ):
        return "PORT_OVERRIDE_UNPROVEN", "native conditional body replaces retail fallback"
    if comparison == "match":
        return "RETAIL_MIPS_MATCH", "normalized instructions and relocation targets match"
    if comparison == "not_run":
        return "UNVERIFIED_NO_MATCHING_BUILD", "fresh matching-object comparison not requested"
    if source.kind == "include_asm":
        return "ASM_FALLBACK_ONLY", "retail ASM exists, but no native C body was found"
    if source.kind == "missing":
        return "MISSING_SOURCE", "no field C definition or INCLUDE_ASM site found"
    return "C_NONMATCHING_UNPROVEN", f"matching comparison: {comparison}"


def build_audit(args: argparse.Namespace) -> dict:
    root = args.root.resolve()
    binary = root / "disc/field.bin"
    if not binary.is_file():
        raise AuditError(f"missing retail field payload: {binary}")
    retail_sha = sha256_file(binary)
    if retail_sha != FIELD_RETAIL_SHA256:
        raise AuditError(
            f"retail field SHA-256 mismatch: got {retail_sha}, expected {FIELD_RETAIL_SHA256}"
        )

    symbols = parse_symbol_addresses(root / "config/symbol_addrs.field.txt")
    field_vram = parse_field_vram(root / "config/field.yaml")
    port_tables = parse_port_tables(root / "pc_port/src/data_field.c")
    retail_tables = read_retail_tables(binary, field_vram, symbols)

    occurrences: dict[str, list[str]] = defaultdict(list)
    table_mismatches: list[dict] = []
    for table, expected_count in EXPECTED_TABLE_LENGTHS.items():
        handlers = port_tables[table]
        if len(handlers) != expected_count:
            raise AuditError(
                f"{table}: got {len(handlers)} entries, expected {expected_count}"
            )
        for index, (handler, retail_address) in enumerate(
            zip(handlers, retail_tables[table], strict=True)
        ):
            occurrences[handler].append(slot_label(table, index))
            source_address = resolve_handler_address(handler, symbols)
            if source_address != retail_address:
                table_mismatches.append(
                    {
                        "table": table,
                        "index": index,
                        "slot": slot_label(table, index),
                        "handler": handler,
                        "source_address": (
                            f"0x{source_address:08X}" if source_address is not None else None
                        ),
                        "retail_address": f"0x{retail_address:08X}",
                    }
                )

    handler_names = set(occurrences)
    source_info = scan_field_sources(root, handler_names)
    stub_path = root / "pc_port/build_native/stubs.c"
    native_object_root = root / "pc_port/build_native/obj"
    native_binary = root / "pc_port/build_native/xeno-port"
    missing_native_artifacts: list[str] = []
    for required in (stub_path, native_binary):
        if not required.is_file():
            missing_native_artifacts.append(str(required.relative_to(root)))
    if not native_object_root.is_dir():
        missing_native_artifacts.append(str(native_object_root.relative_to(root)))
    else:
        expected_field_objects = {
            "src_" + str(path.relative_to(root / "src")).replace("/", "_") + ".o"
            for path in (root / "src/field").rglob("*.c")
        }
        expected_field_objects.update({"field_object_overlay.c.o", "stubs.o"})
        actual_objects = {path.name for path in native_object_root.glob("*.o")}
        missing_native_artifacts.extend(
            str((native_object_root / name).relative_to(root))
            for name in sorted(expected_field_objects - actual_objects)
        )
    native_artifacts_present = not missing_native_artifacts
    native_stubs = parse_generated_stubs(stub_path)
    native_data = parse_generated_data(stub_path)
    authored_stub_functions = scan_authored_stub_functions(root)
    authored_stubs = {row["function"] for row in authored_stub_functions}
    direct_edges = direct_native_stub_edges(root, handler_names, native_stubs)
    direct_authored_edges = direct_native_stub_edges(
        root, handler_names, authored_stubs
    )
    all_field_generated_edges = field_native_generated_edges(
        root, native_stubs | set(native_data)
    )
    all_field_stub_edges = filter_generated_edges(
        all_field_generated_edges, native_stubs
    )
    all_field_data_edges = filter_generated_edges(
        all_field_generated_edges, set(native_data)
    )
    all_field_authored_stub_edges = [
        {
            "caller": row["caller"],
            "object": row["object"],
            "authored_stub_targets": row["generated_stub_targets"],
        }
        for row in field_native_generated_edges(root, authored_stubs)
    ]
    data_callers: dict[str, set[str]] = defaultdict(set)
    for row in all_field_data_edges:
        for target in row["generated_stub_targets"]:
            data_callers[target].add(row["caller"])
    field_data_targets = [
        {
            "symbol": target,
            "allocated_size": native_data[target],
            "caller_count": len(data_callers[target]),
            "callers": sorted(data_callers[target]),
        }
        for target in sorted(data_callers)
    ]
    active_assert_zero = scan_active_assert_zero(root)
    native_active_assert_zero = [
        row for row in active_assert_zero if row["native_may_compile"]
    ]

    current_index: dict[str, list[Symbol]] = {}
    retail_index: dict[str, list[Symbol]] = {}
    comparison_cache: dict[Path, tuple[dict[int, int], dict[int, list[Relocation]]]] = {}
    if args.matching_root is not None:
        current_index, retail_index = locate_matching_objects(args.matching_root.resolve())

    rows: list[HandlerRow] = []
    total_relocation_checks = 0
    for handler in sorted(handler_names, key=lambda item: (occurrences[item][0], item)):
        comparison = "not_run"
        retail_size: int | None = None
        current_size: int | None = None
        relocation_checks = 0
        detail = ""
        if args.matching_root is not None:
            current_symbol = choose_unique_symbol(current_index, handler, "current")
            retail_symbol = choose_unique_symbol(retail_index, handler, "retail")
            if current_symbol is None:
                comparison = "missing_current_object_symbol"
            elif retail_symbol is None:
                comparison = "missing_retail_object_symbol"
            else:
                current_size = current_symbol.size
                retail_size = retail_symbol.size
                comparison, relocation_checks, detail = compare_mips_symbols(
                    current_symbol, retail_symbol, comparison_cache
                )
                if comparison == "match":
                    total_relocation_checks += relocation_checks

        source = source_info[handler]
        direct_stubs = direct_edges.get(handler, [])
        direct_authored_stubs = direct_authored_edges.get(handler, [])
        audit_class, class_detail = classify_row(
            source,
            handler in native_stubs,
            handler in authored_stubs,
            direct_stubs,
            direct_authored_stubs,
            comparison,
        )
        combined_detail = "; ".join(part for part in (class_detail, detail) if part)
        rows.append(
            HandlerRow(
                handler=handler,
                retail_address=(
                    f"0x{resolve_handler_address(handler, symbols):08X}"
                    if resolve_handler_address(handler, symbols) is not None
                    else "UNRESOLVED"
                ),
                slots=occurrences[handler],
                source_path=source.path,
                source_line=source.line,
                source_kind=source.kind,
                source_conditionals=source.conditionals or [],
                source_markers=source.markers or [],
                native_generated_stub=handler in native_stubs,
                native_authored_stub=handler in authored_stubs,
                direct_generated_stub_calls=direct_stubs,
                direct_authored_stub_calls=direct_authored_stubs,
                mips_comparison=comparison,
                retail_size=retail_size,
                current_size=current_size,
                relocation_checks=relocation_checks,
                audit_class=audit_class,
                detail=combined_detail,
            )
        )

    class_counts = Counter(row.audit_class for row in rows)
    comparison_counts = Counter(row.mips_comparison for row in rows)
    source_counts = Counter(row.source_kind for row in rows)
    report = {
        "schema": 5,
        "root": str(root),
        "retail_field": {
            "path": str(binary.relative_to(root)),
            "sha256": retail_sha,
            "vram": f"0x{field_vram:08X}",
        },
        "dispatch": {
            "primary_slots": len(port_tables["g_FieldScriptVMHandlers"]),
            "secondary_slots": len(port_tables["g_FieldScriptVMHandlers2"]),
            "total_slots": sum(len(values) for values in port_tables.values()),
            "unique_handlers": len(handler_names),
            "table_mismatches": table_mismatches,
        },
        "matching_build": {
            "root": str(args.matching_root.resolve()) if args.matching_root else None,
            "comparison_counts": dict(sorted(comparison_counts.items())),
            "relocation_target_checks": total_relocation_checks,
        },
        "native": {
            "artifacts_present": native_artifacts_present,
            "missing_artifacts": missing_native_artifacts,
            "stub_manifest": str(stub_path.relative_to(root)) if stub_path.is_file() else None,
            "generated_stub_count_all_modules": len(native_stubs),
            "generated_data_count_all_modules": len(native_data),
            "authored_stub_functions": authored_stub_functions,
            "dispatched_generated_stubs": sorted(handler_names & native_stubs),
            "dispatched_authored_stubs": sorted(handler_names & authored_stubs),
            "direct_stub_edge_handler_count": len(direct_edges),
            "direct_authored_stub_edge_handler_count": len(direct_authored_edges),
            "field_direct_stub_caller_count": len(all_field_stub_edges),
            "field_direct_stub_edge_count": sum(
                len(row["generated_stub_targets"]) for row in all_field_stub_edges
            ),
            "field_direct_stub_target_count": len(
                {
                    target
                    for row in all_field_stub_edges
                    for target in row["generated_stub_targets"]
                }
            ),
            "field_direct_stub_edges": all_field_stub_edges,
            "field_direct_authored_stub_caller_count": len(
                all_field_authored_stub_edges
            ),
            "field_direct_authored_stub_edge_count": sum(
                len(row["authored_stub_targets"])
                for row in all_field_authored_stub_edges
            ),
            "field_direct_authored_stub_target_count": len(
                {
                    target
                    for row in all_field_authored_stub_edges
                    for target in row["authored_stub_targets"]
                }
            ),
            "field_direct_authored_stub_edges": all_field_authored_stub_edges,
            "field_generated_data_caller_count": len(all_field_data_edges),
            "field_generated_data_edge_count": sum(
                len(row["generated_stub_targets"]) for row in all_field_data_edges
            ),
            "field_generated_data_target_count": len(field_data_targets),
            "field_generated_data_default_32_count": sum(
                row["allocated_size"] == 32 for row in field_data_targets
            ),
            "field_generated_data_edges": all_field_data_edges,
            "field_generated_data_targets": field_data_targets,
            "active_assert_zero": active_assert_zero,
            "native_active_assert_zero": native_active_assert_zero,
        },
        "source_kind_counts": dict(sorted(source_counts.items())),
        "audit_class_counts": dict(sorted(class_counts.items())),
        "handlers": [asdict(row) for row in rows],
    }
    return report


def write_csv_report(report: dict, stream) -> None:
    fields = [
        "slots",
        "handler",
        "retail_address",
        "audit_class",
        "source_kind",
        "source_path",
        "source_line",
        "native_generated_stub",
        "native_authored_stub",
        "direct_generated_stub_calls",
        "direct_authored_stub_calls",
        "mips_comparison",
        "retail_size",
        "current_size",
        "relocation_checks",
        "source_conditionals",
        "source_markers",
        "detail",
    ]
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    for row in report["handlers"]:
        materialized = dict(row)
        for key in (
            "slots",
            "direct_generated_stub_calls",
            "direct_authored_stub_calls",
            "source_conditionals",
            "source_markers",
        ):
            materialized[key] = "|".join(materialized[key])
        writer.writerow({field: materialized[field] for field in fields})


def print_summary(report: dict) -> None:
    dispatch = report["dispatch"]
    matching = report["matching_build"]
    native = report["native"]
    print(
        "FIELD_SCRIPT_VM_AUDIT "
        f"slots={dispatch['total_slots']} unique={dispatch['unique_handlers']} "
        f"table_mismatches={len(dispatch['table_mismatches'])}"
    )
    print(
        "FIELD_SCRIPT_VM_MATCHING "
        + " ".join(
            f"{name}={count}"
            for name, count in sorted(matching["comparison_counts"].items())
        )
        + f" relocation_checks={matching['relocation_target_checks']}"
    )
    print(
        "FIELD_SCRIPT_VM_NATIVE "
        f"artifacts={'present' if native['artifacts_present'] else 'missing'} "
        f"dispatched_stubs={len(native['dispatched_generated_stubs'])} "
        f"dispatched_authored_stubs={len(native['dispatched_authored_stubs'])} "
        f"direct_stub_edges={native['direct_stub_edge_handler_count']} "
        f"direct_authored_edges={native['direct_authored_stub_edge_handler_count']}"
    )
    print(
        "FIELD_SCRIPT_VM_FIELD_DEPS "
        f"callers={native['field_direct_stub_caller_count']} "
        f"edges={native['field_direct_stub_edge_count']} "
        f"targets={native['field_direct_stub_target_count']} "
        f"source_assert_zero={len(native['active_assert_zero'])} "
        f"native_assert_zero={len(native['native_active_assert_zero'])}"
    )
    print(
        "FIELD_SCRIPT_VM_FIELD_AUTHORED "
        f"callers={native['field_direct_authored_stub_caller_count']} "
        f"edges={native['field_direct_authored_stub_edge_count']} "
        f"targets={native['field_direct_authored_stub_target_count']}"
    )
    print(
        "FIELD_SCRIPT_VM_FIELD_DATA "
        f"callers={native['field_generated_data_caller_count']} "
        f"edges={native['field_generated_data_edge_count']} "
        f"targets={native['field_generated_data_target_count']} "
        f"default32={native['field_generated_data_default_32_count']}"
    )
    print(
        "FIELD_SCRIPT_VM_CLASSES "
        + " ".join(
            f"{name}={count}"
            for name, count in sorted(report["audit_class_counts"].items())
        )
    )
    print(
        "FIELD_SCRIPT_VM_GATES "
        f"runtime={'FAIL' if has_strict_runtime_blockers(report) else 'PASS'} "
        f"retail={'FAIL' if has_strict_retail_blockers(report) else 'PASS'}"
    )
    for row in report["handlers"]:
        if row["audit_class"] not in {"RETAIL_MIPS_MATCH"}:
            print(
                f"FIELD_SCRIPT_VM_FINDING slots={','.join(row['slots'])} "
                f"handler={row['handler']} class={row['audit_class']} "
                f"mips={row['mips_comparison']} detail={row['detail']}"
            )


def has_strict_runtime_blockers(report: dict) -> bool:
    """Return whether the statically audited field runtime is incomplete.

    Function/data edges and assert sites are conservative inventories: this
    does not claim that every branch is reachable in an ordinary playthrough.
    Strict mode intentionally fails closed because each site can still replace
    retail behavior if its owning path is reached.
    """

    native = report["native"]
    return (
        not native["artifacts_present"]
        or any(
            row["audit_class"] in STRICT_RUNTIME_BLOCKING_CLASSES
            for row in report["handlers"]
        )
        or bool(native["field_direct_stub_edges"])
        or bool(native["field_direct_authored_stub_edges"])
        or bool(native["field_generated_data_edges"])
        or bool(native["native_active_assert_zero"])
    )


def has_strict_retail_blockers(report: dict) -> bool:
    """Return whether the evidence is insufficient for retail parity.

    Runtime completeness only rejects known placeholders, generated storage,
    and executable assertion gaps. Retail parity additionally requires every
    dispatched handler to have the strongest current evidence class and an
    exact retail dispatch table.
    """

    return (
        bool(report["dispatch"]["table_mismatches"])
        or has_strict_runtime_blockers(report)
        or any(
            row["audit_class"] != "RETAIL_MIPS_MATCH"
            for row in report["handlers"]
        )
    )


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    default_root = Path(__file__).resolve().parents[2]
    parser.add_argument("--root", type=Path, default=default_root)
    parser.add_argument(
        "--matching-root",
        type=Path,
        help="fresh isolated decomp checkout containing build/src/field and build/asm/field",
    )
    parser.add_argument("--json", type=Path, help="write full JSON ledger")
    parser.add_argument("--csv", type=Path, help="write one-row-per-unique-handler CSV")
    parser.add_argument(
        "--strict-runtime",
        action="store_true",
        help=(
            "fail when a dispatched handler is incomplete, any native field "
            "function directly references generated function/data storage, "
            "or field code retains a native-reachable assert(0)"
        ),
    )
    parser.add_argument(
        "--strict-retail",
        action="store_true",
        help=(
            "fail unless every dispatch entry is exact, every handler has "
            "retail MIPS-match evidence, and the strict runtime frontier is clear"
        ),
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv if argv is not None else sys.argv[1:])
    try:
        report = build_audit(args)
    except AuditError as exc:
        print(f"FIELD_SCRIPT_VM_AUDIT_ERROR {exc}", file=sys.stderr)
        return 2

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if args.csv:
        args.csv.parent.mkdir(parents=True, exist_ok=True)
        with args.csv.open("w", encoding="utf-8", newline="") as stream:
            write_csv_report(report, stream)

    print_summary(report)
    failed = bool(report["dispatch"]["table_mismatches"])
    if args.strict_runtime:
        failed |= has_strict_runtime_blockers(report)
    if args.strict_retail:
        failed |= has_strict_retail_blockers(report)
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
