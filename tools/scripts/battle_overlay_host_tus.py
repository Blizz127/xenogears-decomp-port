#!/usr/bin/env python3
"""List the src/battle translation units that own the adopted host leaves.

pc_port/src/battle_overlay_host_leaves.inc is the allowlist: the set of overlay
functions the port is permitted to run as native C instead of interpreting
disc/battle.bin.  This maps that list onto the translation units that must be
compiled into the port for the allowlist to mean anything -- without it,
runtime_bridge_call's dlsym() finds no host body, every target falls back to
the interpreter, and the allowlist is inert.

Ownership rule.  A body may appear twice: once guarded by XENO_PC_PORT in
src/battle/main.c (weak, port-only) and once unconditionally in the TU that
decompiled it.  The unconditional definition wins -- it is what the matching
build compiles.  A port-only definition is accepted when it is the only owner,
which is the normal case for battle: the matching build still assembles the
retail bytes from its own asm segment while the C body is finished for the
port and the differential harnesses.

Usage:
    tools/scripts/battle_overlay_host_tus.py            # one TU per line
    tools/scripts/battle_overlay_host_tus.py --verify    # also report coverage
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BATTLE_SRC = ROOT / "src" / "battle"
LEAVES = ROOT / "pc_port" / "src" / "battle_overlay_host_leaves.inc"
CANDIDATES = ROOT / "pc_port" / "src" / "battle_overlay_host_leaf_candidates.inc"

# A definition, not a declaration: the parameter list is followed by '{' rather
# than ';'.  Mirrors the recogniser in gen_battle_overlay_guest_ram.py.
RETURN_TYPES = r"void|u32|s32|int|long|short|u16|s16|u8|s8|char"
DEFINITION_RE = re.compile(
    rf"^[ \t]*(?:__attribute__\(\(weak\)\)[ \t]*)?(?:static[ \t]+)?"
    rf"({RETURN_TYPES})[ \t]*(\**)[ \t]*(func_\w+)[ \t]*\(",
    re.M,
)
RETURN_WIDTH = {
    "void": 0, "": 0,
    "u8": 8, "s8": 8, "char": 8,
    "u16": 16, "s16": 16, "short": 16,
    "u32": 32, "s32": 32, "int": 32, "long": 32,
}
CONDITIONAL_RE = re.compile(
    r"^[ \t]*#[ \t]*(ifdef|ifndef|if|elif|else|endif)\b[^\n]*", re.M
)


def leaves() -> list[str]:
    return re.findall(r'"(func_\w+)"', LEAVES.read_text())


def _branch_spans(text: str, port: bool) -> list[tuple[int, int]]:
    """Byte ranges whose innermost XENO_PC_PORT branch is taken only with
    (port=True) or only without (port=False) XENO_PC_PORT defined."""
    stack: list[list] = []  # [mentions_port, branch_is_port_only, start]
    spans: list[tuple[int, int]] = []

    def close(end: int) -> None:
        mentions, taken, start = stack[-1]
        if mentions and taken == port:
            spans.append((start, end))

    for m in CONDITIONAL_RE.finditer(text):
        kind = m.group(1)
        if kind in ("ifdef", "ifndef", "if"):
            mentions = "XENO_PC_PORT" in m.group(0)
            # `#ifndef XENO_PC_PORT` selects the branch that is NOT port-only.
            taken = (not mentions) if kind == "ifndef" else mentions
            stack.append([mentions, taken, m.end()])
        elif kind == "else" and stack:
            close(m.start())
            stack[-1][1] = not stack[-1][1]
            stack[-1][2] = m.end()
        elif kind == "elif" and stack:
            close(m.start())
            stack[-1][1] = False
            stack[-1][2] = m.end()
        elif kind == "endif" and stack:
            close(m.start())
            stack.pop()
    return spans


def port_only_spans(text: str) -> list[tuple[int, int]]:
    """Byte ranges whose preprocessor branch is taken only with XENO_PC_PORT."""
    return _branch_spans(text, True)


def retail_only_spans(text: str) -> list[tuple[int, int]]:
    """Byte ranges compiled only WITHOUT XENO_PC_PORT (e.g. `#ifndef
    XENO_PC_PORT` retail-only matched C).  The port never sees these, so they
    cannot own a host leaf."""
    return _branch_spans(text, False)


def in_spans(offset: int, spans: list[tuple[int, int]]) -> bool:
    return any(start <= offset < end for start, end in spans)


def is_definition(text: str, paren_end: int) -> bool:
    """A definition's parameter list is followed by '{'; a declaration by ';'."""
    depth = 0
    for i in range(paren_end, min(len(text), paren_end + 400)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return text[i + 1 :].lstrip().startswith("{")
    return False


def definitions() -> dict[str, list[tuple[str, bool]]]:
    """name -> [(translation unit, is_port_only), ...] over all of src/battle."""
    found: dict[str, list[tuple[str, bool]]] = {}
    for path in sorted(BATTLE_SRC.glob("*.c")):
        text = path.read_text(errors="ignore")
        port_only = port_only_spans(text)
        retail_only = retail_only_spans(text)
        for m in DEFINITION_RE.finditer(text):
            if not is_definition(text, m.end() - 1):
                continue
            if in_spans(m.start(), retail_only):
                continue  # invisible to the port's host build
            found.setdefault(m.group(3), []).append(
                (path.name, in_spans(m.start(), port_only))
            )
    return found


def return_widths() -> dict[str, int]:
    """name -> width in bits of the value the C body returns.

    A pointer return is reported as 0, meaning "do not compare the return
    register": retail returns a guest address there and the host body returns a
    host pointer, so the two are *supposed* to differ. Those bodies have to be
    judged on the guest RAM they change instead.
    """
    widths: dict[str, int] = {}
    for path in sorted(BATTLE_SRC.glob("*.c")):
        text = path.read_text(errors="ignore")
        for m in DEFINITION_RE.finditer(text):
            if not is_definition(text, m.end() - 1):
                continue
            kind, stars, name = m.group(1), m.group(2), m.group(3)
            if stars:
                widths[name] = 0
            else:
                widths.setdefault(name, RETURN_WIDTH.get(kind, 0))
    return widths


def pointer_params() -> dict[str, int]:
    """name -> bitmask of parameter positions declared as a pointer.

    The native bridge translates an argument that looks like a KSEG0/KSEG1
    address into a host pointer, because it cannot tell a pointer from a scalar.
    A sweep that feeds pointer-shaped values to a scalar parameter therefore
    measures that convention, not the body. The differential harness uses this
    mask to keep the two apart.
    """
    masks: dict[str, int] = {}
    for path in sorted(BATTLE_SRC.glob("*.c")):
        text = path.read_text(errors="ignore")
        for m in DEFINITION_RE.finditer(text):
            if not is_definition(text, m.end() - 1):
                continue
            name = m.group(3)
            depth = 0
            end = m.end() - 1
            for i in range(end, min(len(text), end + 400)):
                if text[i] == "(":
                    depth += 1
                elif text[i] == ")":
                    depth -= 1
                    if depth == 0:
                        end = i
                        break
            params = text[m.end() : end]
            mask = 0
            for index, param in enumerate(params.split(",")):
                if index >= 8:
                    break
                if "*" in param:
                    mask |= 1 << index
            masks[name] = mask
    return masks


def owners(name: str, found: dict[str, list[tuple[str, bool]]]) -> list[str]:
    entries = found.get(name, [])
    matching = [tu for tu, port_only in entries if not port_only]
    chosen = matching or [tu for tu, _ in entries]
    return sorted(set(chosen))


CALL_RE = re.compile(r"\b(func_\w+)\b")


def c_bodies() -> dict[str, set[str]]:
    """name -> the func_* names its body refers to, over all of src/battle.

    Any reference counts, not just a call: taking the address of a function and
    dispatching through it later reaches the same placeholder, and retail would
    have run real code there.
    """
    bodies: dict[str, set[str]] = {}
    for path in sorted(BATTLE_SRC.glob("*.c")):
        text = path.read_text(errors="ignore")
        for m in DEFINITION_RE.finditer(text):
            if not is_definition(text, m.end() - 1):
                continue
            depth = 0
            start = m.end() - 1
            end = start
            for i in range(start, len(text)):
                if text[i] == "{":
                    depth += 1
                elif text[i] == "}":
                    depth -= 1
                    if depth == 0:
                        end = i
                        break
            body = re.sub(r"/\*.*?\*/", "", text[start:end], flags=re.S)
            body = re.sub(r"//.*", "", body)
            bodies[m.group(3)] = set(CALL_RE.findall(body))
    return bodies


JAL_RE = re.compile(r"jal\s+([A-Za-z]\w+)")


def asm_calls() -> dict[str, set[str]]:
    """name -> jal targets from its retail asm, when present.

    A function without a C body is opaque to c_bodies(), but retail still
    calls through it: adopting a leaf above one hides a whole cascade of
    undecompiled callees (measured once at 14 bodies for a single leaf).
    The asm edge keeps the gate honest about that cascade. Labels (.L*)
    never match the pattern; engine/libGPU targets become graph leaves.
    """
    calls: dict[str, set[str]] = {}
    for sub in ("nonmatchings", "matchings"):
        base = ROOT / "asm" / "battle" / sub
        if not base.is_dir():
            continue
        for path in sorted(base.glob("*/*.s")):
            name = path.stem
            if name in calls:
                continue
            text = path.read_text(errors="ignore")
            calls[name] = set(JAL_RE.findall(text))
    return calls


def stub_names(path: str) -> set[str]:
    """Symbols the generated stub manifest defines, i.e. have no C body."""
    text = Path(path).read_text(errors="ignore")
    section = text.split("xeno_port_is_generated_stub", 1)
    if len(section) < 2:
        return set()
    return set(re.findall(r'"(\w+)"', section[1]))


def reachable_stubs(leaf: str, bodies: dict[str, set[str]],
                    stubs: set[str],
                    asm: dict[str, set[str]] | None = None) -> set[str]:
    """Stub-backed functions reachable from a leaf.

    C-body edges first; a name without a C body falls through to its retail
    asm jal targets, so a leaf above an undecompiled function is still
    charged with the cascade underneath it. Names with neither a body nor
    asm (engine TUs, libGPU) are opaque leaves: they resolve outside battle.
    """
    seen: set[str] = set()
    stack = list(bodies.get(leaf, ()))
    hits: set[str] = set()
    while stack:
        name = stack.pop()
        if name in seen:
            continue
        seen.add(name)
        if name in stubs:
            hits.add(name)
            continue
        if name in bodies:
            stack.extend(bodies[name])
        elif asm is not None:
            stack.extend(asm.get(name, ()))
    return hits


def main() -> int:
    found = definitions()
    wanted = leaves()
    tus: list[str] = []
    missing: list[str] = []
    ambiguous: list[str] = []
    for name in wanted:
        where = owners(name, found)
        if not where:
            missing.append(name)
            continue
        if len(where) > 1:
            ambiguous.append(f"{name} in {'/'.join(where)}")
            continue
        if where[0] not in tus:
            tus.append(where[0])

    if missing or ambiguous:
        for name in missing:
            print(
                f"ERROR: adopted leaf {name} has no C definition in src/battle",
                file=sys.stderr,
            )
        for item in ambiguous:
            print(f"ERROR: adopted leaf has rival owners: {item}", file=sys.stderr)
        print(
            "ERROR: the allowlist and src/battle disagree; refusing to emit a TU list.",
            file=sys.stderr,
        )
        return 1

    if "--leaves" in sys.argv:
        for name in wanted:
            print(name)
        return 0

    if "--leaf-types" in sys.argv:
        widths = return_widths()
        masks = pointer_params()
        for name in wanted:
            print(
                f'    {{ "{name}", {widths.get(name, 0)}, {masks.get(name, 0)} }},'
            )
        return 0

    if "--pointer-globals" in sys.argv:
        # Guest addresses whose overlay alias is a *pointer*: the guest word
        # holds a PSX address that a body dereferences. If the sweep leaves one
        # of those zero, the retail side faults on the null dereference and the
        # case can never prove anything; seeding the word with a live RAM
        # address is what turns such a body into comparable cases. The
        # zero-filled pattern still runs, so null handling is not hidden -- an
        # unprovable null case stays inconclusive instead of being papered over.
        header = (ROOT / "pc_port" / "src" / "battle_overlay_guest_ram.h").read_text()
        pattern = re.compile(
            r"#define (D_\w+) \(\(\w+ \*\)PSX_ADDR\(\*\(u32 \*\)PSX_ADDR"
            r"\((0x[0-9A-Fa-f]+)\)\)\)"
        )
        seen: set[str] = set()
        for line in header.splitlines():
            m = pattern.match(line)
            if m is None or m.group(1) in seen:
                continue
            seen.add(m.group(1))
            print(f"    0x{m.group(2)[2:]},  /* {m.group(1)} */")
        return 0

    if "--check-stubs" in sys.argv:
        manifest = sys.argv[sys.argv.index("--check-stubs") + 1]
        stubs = stub_names(manifest)
        bodies = c_bodies()
        asm = asm_calls()
        violations = 0
        for name in wanted:
            hits = reachable_stubs(name, bodies, stubs, asm)
            for hit in sorted(hits):
                print(f"REACHES-STUB {name} -> {hit}")
                violations += 1
        if violations:
            print(
                f"ERROR: {violations} adopted-leaf call path(s) reach a generated "
                "stub. An adopted body must not call a placeholder: retail would "
                "have run real code there.",
                file=sys.stderr,
            )
            return 1
        print(
            f"# {len(wanted)} adopted leaves reach no generated stub "
            f"({len(stubs)} stubs checked)",
            file=sys.stderr,
        )
        return 0

    if "--eligible" in sys.argv:
        # Candidates for the next adoption batch, filtered by rules rather than
        # by hand: present in the generated candidate set, not already adopted,
        # no call path that reaches a generated stub, and no pointer parameter.
        # A pointer-parameter body can only be reached by a hand written case,
        # so the differential sweep cannot vouch for it yet.
        manifest = sys.argv[sys.argv.index("--eligible") + 1]
        stubs = stub_names(manifest)
        bodies = c_bodies()
        asm = asm_calls()
        masks = pointer_params()
        adopted = set(wanted)
        candidates = re.findall(r'"(func_\w+)"', CANDIDATES.read_text())
        for candidate in candidates:
            if candidate in adopted:
                continue
            if masks.get(candidate, 0) != 0:
                continue
            if candidate not in bodies:
                continue
            if reachable_stubs(candidate, bodies, stubs, asm):
                continue
            # Body closure: stub-reach is not enough. A battle function with
            # retail asm but no C body is not a stub *yet* -- nothing
            # references it -- but adopting above it promotes the whole
            # cascade into the link (measured once at 14 bodies for one
            # leaf), where the next trial link stubs it and the build gate
            # fires. Names with neither body nor asm resolve outside battle
            # (engine TUs, libGPU) and are opaque here.
            # NOTE: the --check-stubs build gate deliberately does not apply
            # this rule: it runs against the fresh trial-link manifest, where
            # every such cascade node already materialized as a stub.
            seen: set[str] = set()
            stack = list(bodies[candidate])
            uncovered = False
            while stack:
                name = stack.pop()
                if name in seen:
                    continue
                seen.add(name)
                if name in bodies:
                    stack.extend(bodies[name])
                    continue
                if name not in asm:
                    continue
                uncovered = True
                break
            if uncovered:
                continue
            print(candidate)
        return 0

    if "--referenced" in sys.argv:
        # Every name the adopted leaves reach through battle C bodies, falling
        # through to retail asm where there is no body yet. A symbol in this
        # set must keep its real owner: redirecting one silently changes what
        # an adopted body calls.
        bodies = c_bodies()
        asm = asm_calls()
        seen: set[str] = set()
        for leaf in wanted:
            stack = list(bodies.get(leaf, ()))
            while stack:
                name = stack.pop()
                if name in seen:
                    continue
                seen.add(name)
                if name in bodies:
                    stack.extend(bodies[name])
                else:
                    stack.extend(asm.get(name, ()))
        for name in wanted:
            seen.discard(name)
        for name in sorted(seen):
            print(name)
        return 0

    for tu in sorted(tus):
        print(f"src/battle/{tu}")
    if "--verify" in sys.argv:
        print(
            f"# {len(wanted)} adopted leaves across {len(tus)} translation units",
            file=sys.stderr,
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
