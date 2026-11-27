#!/usr/bin/env python3
"""Scan tracked source and optional artifacts for known restricted content."""

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import zipfile
from itertools import chain

from public_digest_address_dispatch import check_address_dispatch
from public_guard_archives import archive_inputs
from public_world_cohort import ReleaseRosters, file_reason
from public_digest_bodies import check_bodies
from public_digest_packaging import check_packaging
from public_digest_sdk import check_sdk
from public_digest_short import check_short
from public_digest_translation import check_translation
from public_digest_translation_data import check_translation_data


def git(root, *args):
    return subprocess.check_output(["git", "-C", root, *args])


def index_entries(root):
    for entry in git(root, "ls-files", "--stage", "-z").split(b"\0"):
        if not entry:
            continue
        metadata, name = entry.split(b"\t", 1)
        mode, oid, stage = metadata.split()
        if stage != b"0":
            raise ValueError(f"unmerged index entry {os.fsdecode(name)!r}")
        yield mode, oid, os.fsdecode(name)


def tree_entries(root, revision):
    tree = git(root, "rev-parse", "--verify", "--end-of-options", revision + "^{tree}").strip()
    for entry in git(root, "ls-tree", "-rz", tree).split(b"\0"):
        if entry:
            metadata, name = entry.split(b"\t", 1)
            mode, _, oid = metadata.split()
            yield mode, oid, os.fsdecode(name)


def object_inputs(root, entries):
    with subprocess.Popen(["git", "-C", root, "cat-file", "--batch"],
                          stdin=subprocess.PIPE, stdout=subprocess.PIPE) as process:
        try:
            for mode, oid, name in entries:
                if mode == b"160000":
                    yield name, b""
                    continue
                process.stdin.write(oid + b"\n")
                process.stdin.flush()
                identity = process.stdout.readline().split()
                if len(identity) != 3 or identity[:2] != [oid, b"blob"]:
                    raise ValueError(f"cannot read Git object for {name!r}")
                size = int(identity[2])
                data = process.stdout.read(size)
                if len(data) != size or process.stdout.read(1) != b"\n":
                    raise ValueError(f"incomplete Git object for {name!r}")
                yield name, data
        finally:
            process.stdin.close()
        if process.wait():
            raise ValueError("git cat-file failed")


def worktree_inputs(root, entries):
    for mode, _, name in entries:
        path = root / name
        if mode == b"160000":
            yield name, b""
        elif not path.parent.resolve().is_relative_to(root):
            raise ValueError(f"tracked file has a parent outside the checkout: {name!r}")
        elif path.is_symlink():
            yield name, os.fsencode(os.readlink(path))
        elif path.is_file():
            yield name, path.read_bytes()
        elif path.exists():
            raise ValueError(f"expected a regular file for {name!r}")
        # A tracked file deleted from the working tree has no working bytes.


def reasons(name, data):
    checks = (file_reason, check_sdk, check_bodies, check_short, check_packaging,
              check_translation, check_translation_data, check_address_dispatch)
    return [reason for check in checks for reason in check(name, data)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--staged", action="store_true", help="read the index, including staged-only bytes")
    mode.add_argument("--revision", metavar="REV", help="read an exact Git tree")
    parser.add_argument("--binary", action="append", type=Path, default=[],
                        help="also scan one built file (repeatable)")
    args = parser.parse_args()
    try:
        root = Path(os.fsdecode(git(Path.cwd(), "rev-parse", "--show-toplevel").rstrip(b"\n"))).resolve()
        if args.revision:
            inputs = object_inputs(root, tree_entries(root, args.revision))
        elif args.staged:
            inputs = object_inputs(root, index_entries(root))
        else:
            inputs = worktree_inputs(root, index_entries(root))
        artifacts = ((binary.name, binary.read_bytes()) for binary in args.binary)
        files = hits = 0
        rosters = ReleaseRosters()
        for path, content in chain(inputs, artifacts):
            for name, data in archive_inputs(path, content):
                files += 1
                rosters.visit(name, data)
                for reason in reasons(name, data):
                    print(f"{name!r}: {reason}")
                    hits += 1
        for name, reason in rosters.findings():
            print(f"{name!r}: {reason}")
            hits += 1
        print(f"Public source regression scan checked {files} paths and found {hits} matches.")
        return int(hits != 0)
    except (OSError, ValueError, EOFError, RuntimeError, tarfile.TarError, zipfile.BadZipFile, subprocess.CalledProcessError) as error:
        print(f"Public source regression scan could not finish: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
