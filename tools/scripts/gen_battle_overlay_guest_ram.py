#!/usr/bin/env python3
"""Map battle-overlay D_* symbols onto guest RAM for the native port.

Matching still uses the externs.  Under XENO_PC_PORT, overlay-range arrays
and scalars become PSX_ADDR aliases so decompiled C writes the loaded
battle.bin image instead of a second host copy.  Pointer-typed overlay
symbols load the guest word and translate that too.
"""
from __future__ import annotations

import re
from collections import OrderedDict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BATTLE_SRC = ROOT / "src" / "battle"
HEADER = ROOT / "pc_port" / "src" / "battle_overlay_guest_ram.h"
CANDIDATES = ROOT / "pc_port" / "src" / "battle_overlay_host_leaf_candidates.inc"
OVERLAY_LO = 0x8006FAF0
OVERLAY_HI = 0x800E0000

EXTERN_RE = re.compile(r"^extern\s+([^;]+);", re.M)
FUNC_RE = re.compile(
    r"(?:__attribute__\(\(weak\)\)\s*)*(?:static\s+)?"
    r"(?:void|u32|s32|int|long|short|u16|s16|u8|s8|char|u32\s+\*|void\s+\*|"
    r"u8\s+\*|s16\s+\*|u16\s+\*)\s+(func_\w+)\s*\(([^)]*)\)\s*\{",
    re.S,
)
CALL_RE = re.compile(
    r"\b(func_\w+|SetSemiTrans|SetShadeTex|LoadImage|DrawSync|HeapAlloc|"
    r"HeapFree|Archive\w+|WorkList\w+|TimerWorkList\w+|rand|ratan2|rcos|"
    r"rsin|GetTPage|GetClut)\s*\("
)
NESTED_RE = re.compile(
    r"\*\(\s*(?:u8|u16|u32|s16|s32|void)\s*\*\s*\)\s*\(\s*\*\s*\(\s*u32\s*\*\s*\)"
    r"|\*\(\s*u8\s*\*\*\s*\)"
    r"|u8\s*\*\*"
)


def overlay_addr(name: str) -> int | None:
    if not re.match(r"D_8[0-9A-Fa-f]{7}$", name):
        return None
    value = int(name[2:], 16)
    if OVERLAY_LO <= value < OVERLAY_HI:
        return value
    return None


# One declarator out of an extern's comma-separated list: optional leading
# '*'s, the name, then optional array dimensions.  `[]` (unsized) marks the
# outermost dimension; any sized dimensions after it belong to the element
# type, so `D[][28]` has element type `base (*)[28]`, not `base *`.
DECLARATOR_RE = re.compile(r"^(\**)\s*(\w+)\s*((?:\[[^\]]*\])*)$")
TAG_KEYWORDS = ("struct", "union", "enum")
# Tokens that continue a multi-word base type rather than starting a
# declarator, so `unsigned long D_x` keeps both words.
TYPE_KEYWORDS = ("unsigned", "signed", "long", "short", "int", "char", "volatile")


def split_declarators(raw: str) -> tuple[str, list[str]]:
    """Split `base type d1, d2, d3` into its base type and declarator list.

    The base type ends at the first declarator, which is the first token
    sequence containing an identifier that is not a type keyword.  Splitting
    the remainder on commas is safe here: these are plain object declarations
    with no function-pointer parameter lists.
    """
    raw = re.sub(r"\s+", " ", raw).replace("const", "").strip()
    # Whitespace is dropped: a declarator only needs its stars, name and
    # dimensions, and keeping spaces would break `struct <tag>` pairing.
    tokens = re.findall(r"\*+|\w+|[\[\],]|\S", raw)
    base: list[str] = []
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok in TAG_KEYWORDS:
            # `struct Foo` contributes two tokens to the base type.
            base.append(tok)
            i += 1
            if i < len(tokens) and re.match(r"^\w+$", tokens[i]):
                base.append(tokens[i])
                i += 1
            continue
        if re.match(r"^\w+$", tok):
            # A bare identifier continues the base type only while the base
            # is still empty or the token is a type keyword (`unsigned long`).
            if base and tok not in TYPE_KEYWORDS:
                break
            base.append(tok)
            i += 1
            continue
        break
    return " ".join(base), [d for d in "".join(tokens[i:]).split(",") if d]


def parse_externs(raw: str) -> list[tuple[str, str, str]]:
    """Every overlay-range D_* declared by one extern statement.

    Returns (name, kind, element_type) per declarator.  The previous version
    only ever looked at the last name in the statement and treated everything
    textually before it as the type, so in `extern u8 A, B, C;` only C got a
    macro and its "type" came out as `u8 A, B,`.
    """
    base, declarators = split_declarators(raw)
    if not base:
        return []
    out: list[tuple[str, str, str]] = []
    for decl in declarators:
        m = DECLARATOR_RE.match(decl)
        if m is None:
            continue
        stars, name, dims = m.group(1), m.group(2), m.group(3)
        if overlay_addr(name) is None:
            continue
        sized = re.findall(r"\[([^\]]+)\]", dims)
        if stars:
            kind = "ptr"
            elem = f"{base} {'*' * (len(stars) - 1)}".strip()
        elif dims:
            kind = "array"
            # `[]` then `[N]...` -> element type is an array of N.
            elem = base
            if sized:
                elem = f"{base} (*)[{']['.join(sized)}]"
        else:
            kind = "scalar"
            elem = base
        out.append((name, kind, elem))
    return out


def parse_extern(raw: str) -> tuple[str, str, str] | None:
    """First overlay symbol declared by this extern, or None.

    Kept for the callers that only need "does this statement declare one of
    the symbols we alias".
    """
    parsed = parse_externs(raw)
    return parsed[0] if parsed else None


def extract_body(text: str, brace: int) -> str:
    depth = 0
    for j, ch in enumerate(text[brace:], brace):
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1 : j]
    return ""


def collect_decls() -> OrderedDict:
    decls: OrderedDict[str, tuple[str, str]] = OrderedDict()
    for path in sorted(BATTLE_SRC.glob("*.c")):
        text = path.read_text(errors="ignore")
        for match in EXTERN_RE.finditer(text):
            for name, kind, type_tok in parse_externs(match.group(1)):
                rank = {"scalar": 0, "array": 1, "ptr": 2}
                old = decls.get(name)
                # NOTE: several addresses are declared with genuinely
                # conflicting types across TUs -- `u8 D_800D32A1[]` in one and
                # `u8 D_800D32A1[][8]` in another -- because each TU takes a
                # different view of the same bytes and indexes accordingly.
                # One shared macro cannot satisfy both, so keep the existing
                # first-highest-rank-wins resolution rather than inventing a
                # new tie-break: changing it silently alters the pointer
                # arithmetic of whichever TU loses.
                if old is None or rank[kind] > rank[old[0]]:
                    decls[name] = (kind, type_tok)
                # Tried and REVERTED: preferring a named type (`struct
                # BattleCommandContext *`) over the `u8 *` placeholder.  The
                # struct is defined in exactly one TU, so that cast broke the
                # 12 other TUs that legitimately use the byte-pointer view --
                # including main36.c, which owns adopted leaves.  A forward
                # declaration is not enough because those TUs dereference it.
                # Per-TU types cannot be expressed in one shared header; see
                # docs/evidence/battle-guest-ram-generator-20260921/.
    return decls


def wrap_externs(names: set[str]) -> int:
    changed = 0
    for path in sorted(BATTLE_SRC.glob("*.c")):
        text = path.read_text()
        original = text
        # An extern glued to the end of the previous statement, e.g.
        # `}extern u32 D_800D3368[];`, is invisible to the line-oriented
        # wrapper below, so it never gets its #ifndef guard.  In host-bodies
        # mode the declaration then survives alongside the generated macro and
        # the macro expands *inside the declaration*, which is what produced
        # "expected ')' before '*'" at the macro's own definition line.  Put
        # such an extern on its own line before wrapping.
        text = re.sub(r"(?m)([;}])[ \t]*(extern\s)", r"\1\n\2", text)

        def repl(match: re.Match) -> str:
            line = match.group(0)
            parsed = parse_extern(match.group(1))
            if parsed is None or parsed[0] not in names:
                return line
            if line.strip().startswith("#"):
                return line
            return f"#ifndef XENO_PC_PORT\n{line}\n#endif"

        # Only wrap standalone extern lines that are not already guarded.
        lines = text.splitlines(keepends=True)
        out = []
        i = 0
        while i < len(lines):
            line = lines[i]
            stripped = line.strip()
            m = re.match(r"extern\s+([^;]+);", stripped)
            if (
                m
                and i > 0
                and lines[i - 1].strip() == "#ifndef XENO_PC_PORT"
            ):
                out.append(line)
                i += 1
                continue
            # Wrap if ANY declarator in the statement is aliased: in
            # `extern u8 A, B, C;` the aliased symbol may not be the first.
            parsed = parse_externs(m.group(1)) if m else []
            if any(n in names for n, _, _ in parsed):
                out.append("#ifndef XENO_PC_PORT\n")
                out.append(line if line.endswith("\n") else line + "\n")
                out.append("#endif\n")
                changed += 1
                i += 1
                continue
            out.append(line)
            i += 1
        new = "".join(out)
        if new != original:
            path.write_text(new)
    return changed


def emit_header(decls: OrderedDict) -> None:
    lines = [
        "/* Generated by tools/scripts/gen_battle_overlay_guest_ram.py — do not edit. */",
        "#ifndef XENO_BATTLE_OVERLAY_GUEST_RAM_H",
        "#define XENO_BATTLE_OVERLAY_GUEST_RAM_H",
        "",
        "#include \"types.h\"",
        "",
        "/* Overlay-range D_* live in the loaded battle.bin image. Arrays and",
        " * scalars alias guest RAM. Pointer-typed symbols load the guest word",
        " * and translate that address too. */",
        "",
    ]
    # A struct tag is often defined in only one TU, but every TU including
    # this header has to be able to *parse* the cast.  Forward declarations
    # make the pointer cast legal everywhere; member access still only
    # compiles where the full definition is in scope, which is what we want.
    tags = set()
    for _, tok in decls.values():
        parts = tok.split()
        if len(parts) >= 2 and parts[0] in TAG_KEYWORDS:
            tags.add(f"{parts[0]} {parts[1]}")
    if tags:
        lines += [f"{tag};" for tag in sorted(tags)] + [""]
    for name, (kind, type_tok) in decls.items():
        addr = overlay_addr(name)
        assert addr is not None
        hex_addr = f"0x{addr:08X}"
        # `u8 (*)[28]` is already a complete pointer type; anything else needs
        # a `*` appended to point at the guest object.
        pointer_cast = type_tok if "(*)" in type_tok else f"{type_tok} *"
        if kind == "ptr":
            lines.append(
                f"#define {name} (({pointer_cast})PSX_ADDR(*(u32 *)PSX_ADDR({hex_addr})))"
            )
        elif kind == "array":
            lines.append(f"#define {name} (({pointer_cast})PSX_ADDR({hex_addr}))")
        else:
            lines.append(f"#define {name} (*(({pointer_cast})PSX_ADDR({hex_addr})))")
    lines += ["", "#endif", ""]
    HEADER.write_text("\n".join(lines))


def adoptable(decls: OrderedDict) -> list[str]:
    mapped = set(decls)
    names = []
    already = {
        "func_80079934",
        "func_80089B50",
        "func_800A3484",
        "func_800AEEEC",
        "func_800B16A4",
        "func_800B6930",
        "func_800B6990",
        "func_800B69E4",
    }
    for path in sorted(BATTLE_SRC.glob("*.c")):
        text = path.read_text(errors="ignore")
        for match in FUNC_RE.finditer(text):
            name = match.group(1)
            body = extract_body(text, match.end() - 1)
            code = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
            code = re.sub(r"//.*", "", code)
            stripped = code.strip()
            if stripped in ("", "return;", "return 0;", "return 0u;"):
                continue
            if NESTED_RE.search(match.group(2) + code):
                continue
            if re.search(r"\*\(\s*u32\s*\*\s*\)\s*\([^)]+\)", code):
                continue
            used = set(re.findall(r"\b(D_\w+)\b", code))
            if any(overlay_addr(n) is not None and n not in mapped for n in used):
                continue
            names.append(name)
    # unique, keep order, include already
    seen = set()
    out = []
    for n in list(already) + names:
        if n not in seen:
            seen.add(n)
            out.append(n)
    return out


def main() -> None:
    decls = collect_decls()
    emit_header(decls)
    wrapped = wrap_externs(set(decls))
    leaves = adoptable(decls)
    CANDIDATES.write_text("".join(f'    "{n}",\n' for n in leaves))
    print(f"symbols {len(decls)} wrapped-externs {wrapped} candidates {len(leaves)}")
    print(f"wrote {HEADER.relative_to(ROOT)}")
    print(f"wrote {CANDIDATES.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
