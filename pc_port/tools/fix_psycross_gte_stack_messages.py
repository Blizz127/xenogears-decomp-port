#!/usr/bin/env python3
"""Replace SDK matrix-stack diagnostics without embedding retail text.

Accept pristine PsyCross or the old insertion patch's malformed error blocks.
Keep successful stack operations verbatim, refuse unfamiliar layouts, and make
the repair idempotent. An ordinary deletion diff would retain the SDK strings
in the repository, so only their structural locations are specified here.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re


MESSAGE = {
    "PushMatrix": "[psycross] PushMatrix: matrix stack full (20 entries)",
    "PopMatrix": "[psycross] PopMatrix: matrix stack empty",
}
SUCCESS_END = {
    "PushMatrix": "\t\tmatrixLevel++;\n\t}\n",
    "PopMatrix": "\t\tgte_SetTransMatrix(m);\n\t}\n",
}
# A single string argument, with ordinary C escapes; no text from the SDK.
PRINT = r'\t\tprintf\("(?:[^"\\\n]|\\.)*"\);\n'
ORIGINAL_ERROR = re.compile(r"\telse\n\t{\n" + PRINT + r"\t}\n")
OLD_ERROR = re.compile(
    r"#if 0 // _xeno_gte_stack_messages: the SDK text is not shipped\n"
    r"\telse\n#endif\n" + PRINT + r"\t{\n" + PRINT + r"\t}\n"
)


def repair(source: str) -> tuple[str, int]:
    changed = 0
    for name in MESSAGE:
        pattern = re.compile(r"(?m)^void " + name + r"\(\)\n\{\n(.*?)^\}", re.S)
        matches = list(pattern.finditer(source))
        if len(matches) != 1:
            raise ValueError(f"expected one {name} definition, found {len(matches)}")
        match = matches[0]
        body = match.group(1)
        end = SUCCESS_END[name]
        if body.count(end) != 1:
            raise ValueError(f"{name}: unfamiliar successful-operation block")
        cut = body.index(end) + len(end)
        prefix, suffix = body[:cut], body[cut:]
        replacement = (
            "\telse\n\t{\n"
            "\t\t// _xeno_gte_stack_messages_v2: host diagnostic on failure only.\n"
            f'\t\tfputs("{MESSAGE[name]}\\n", stdout);\n'
            "\t}\n"
        )
        if suffix == replacement:
            continue
        if not (ORIGINAL_ERROR.fullmatch(suffix) or OLD_ERROR.fullmatch(suffix)):
            raise ValueError(f"{name}: unfamiliar error block; source left unchanged")
        source = source[:match.start(1)] + prefix + replacement + source[match.end(1):]
        changed += 1
    return source, changed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("source", type=Path, help="PsyCross src/psx/LIBGTE.C")
    args = parser.parse_args()
    # Complete both recognitions before writing, including on a mixed tree.
    # Vendor comments contain legacy non-UTF-8 bytes; round-trip them exactly.
    source = args.source.read_bytes().decode("latin1")
    try:
        updated, count = repair(source)
    except ValueError as exc:
        parser.error(str(exc))
    if count:
        args.source.write_bytes(updated.encode("latin1"))
    print(f"PsyCross matrix-stack diagnostics: {count} repaired")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
