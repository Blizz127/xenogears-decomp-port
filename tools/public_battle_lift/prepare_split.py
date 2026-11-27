#!/usr/bin/env python3
# Copyright (c) 2026 Blizz127 and contributors
# SPDX-License-Identifier: MIT
"""Prepare a private, isolated Splat workspace from a checked-out project and an owned overlay."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from generate import checked, PROFILE_SHA256


def prepare(project, overlay, output, splat):
    profile = json.loads(checked(Path(__file__).with_name("us-disc1.json"), PROFILE_SHA256))
    hashes = profile["file_hashes"]
    overlay_bytes = checked(overlay, hashes["disc/battle.bin"])
    if output.exists():
        raise ValueError(f"use the existing split or choose a new directory: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".battle-split-", dir=output.parent) as staging:
        work = Path(staging) / "work"
        work.mkdir()
        for name, digest in hashes.items():
            if name.startswith(("config/", "tools/")):
                target = work / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(checked(project / name, digest))
        (work / "disc").mkdir()
        (work / "disc/battle.bin").write_bytes(overlay_bytes)
        program = shutil.which(splat)
        if not program:
            raise ValueError("splat executable was not found")
        subprocess.run([str(Path(program).resolve()), "split", "config/battle.yaml"],
                       cwd=work, check=True, env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1"})
        for name, digest in hashes.items():
            checked(work / name, digest)
        work.rename(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", required=True, type=Path)
    parser.add_argument("--overlay", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--splat", default="splat")
    args = parser.parse_args()
    try:
        prepare(args.project, args.overlay, args.out, args.splat)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"{parser.prog}: {exc}\n")


if __name__ == "__main__":
    main()
