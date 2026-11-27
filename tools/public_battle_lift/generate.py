#!/usr/bin/env python3
# Copyright (c) 2026 Blizz127 and contributors
# SPDX-License-Identifier: MIT
"""Generate one native battle module from a US Disc 1 split using the public lifter."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

PROFILE_SHA256 = "268db81d864a2b54d3947eda65556731066c4ccccb6c5fd233a6f5cff36ac9ea"
CASE = re.compile(r"\bcase 0x([0-9a-fA-F]{8})u:")
ENTRY = re.compile(r"^\s*(?:glabel|\.glabel|\.globl)\s+func_([0-9A-Fa-f]{8})\b", re.MULTILINE)
PREFIX = """/* Generated locally from the player's battle overlay. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_lift.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

"""
SUFFIX = "\n#pragma GCC diagnostic pop\n#endif\n"


def checked(path, expected):
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise ValueError(f"unexpected contents of {path}")
    return data


def relative_switches(text, entry):
    """Keep indirect branch case values relative to the native routine's guest entry."""
    output = []
    for line in text.splitlines(keepends=True):
        if "switch (t_)" in line:
            if len(CASE.findall(line)) != line.count("case ") or not CASE.search(line):
                raise ValueError("unrecognized output from the jump-table generator")
            def rebase(match):
                destination = int(match.group(1), 16)
                if destination < entry:
                    raise ValueError("case precedes the selected routine")
                return f"case 0x{destination-entry:X}u:"
            line = CASE.sub(rebase, line).replace("switch (t_)", f"switch (t_ - 0x{entry:08X}u)")
        output.append(line)
    return "".join(output)


def generate(root, group):
    profile = json.loads(checked(Path(__file__).with_name("us-disc1.json"), PROFILE_SHA256))
    for name, digest in profile["file_hashes"].items():
        checked(root / name, digest)
    selection = profile["groups"][group]
    expected_names = {"func_" + row["entry"] + ".s" for row in selection}
    listings = {}
    for path in (root / "asm" / "battle" / "nonmatchings").glob("*/*.s"):
        if path.name in expected_names:
            if path.name in listings:
                raise ValueError(f"ambiguous listing for {path.name}")
            listings[path.name] = path
    if set(listings) != expected_names:
        raise ValueError("incomplete local Splat output")
    pieces = [PREFIX]
    if group in ("frame", "status"):
        pieces.append(f"BL_EXPORT_FRAME_TOP(mainasm_{group}_port)\n")
    for row in selection:
        entry = row["entry"]
        symbol = "func_" + entry
        path = listings[symbol + ".s"]
        listing = checked(path, row["listing_sha256"]).decode("ascii")
        label = ENTRY.search(listing)
        if not label or label.group(1).upper() != entry:
            raise ValueError(f"missing definition at the selected address {entry}")
        command = [sys.executable, str(root / "tools/scripts/battle_lift_asm.py"),
                   str(path), "--args", str(row["arguments"]), "--jal-service"]
        result_type = row["return_type"]
        if result_type == "void":
            command.append("--void")
        run = subprocess.run(command, cwd=root, capture_output=True, check=False)
        if run.returncode:
            raise ValueError(f"native translation failed for {symbol}: {run.stderr.decode(errors='replace').strip()}")
        code = relative_switches(run.stdout.decode("utf-8"), int(entry, 16))
        if result_type in ("s16", "u16", "u8"):
            original = "u32 " + symbol + "("
            if code.count(original) != 1:
                raise ValueError(f"unexpected return signature for {symbol}")
            code = code.replace(original, result_type + " " + symbol + "(")
        pieces.extend(("\n", code))
    pieces.append(SUFFIX)
    return "".join(pieces).encode("utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True, type=Path, help="project and local split directory")
    parser.add_argument("--group", required=True, choices=("cmdmenu", "frame", "status"))
    parser.add_argument("--out", required=True, type=Path, help="local, untracked compiler input")
    args = parser.parse_args()
    try:
        data = generate(args.root.resolve(), args.group)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(dir=args.out.parent, delete=False) as handle:
            temporary = Path(handle.name)
            try:
                handle.write(data)
                handle.close()
                os.replace(temporary, args.out)
            finally:
                temporary.unlink(missing_ok=True)
    except (OSError, ValueError) as exc:
        parser.exit(1, f"{parser.prog}: {exc}\n")


if __name__ == "__main__":
    main()
