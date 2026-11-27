#!/usr/bin/env python3
"""Restore the Xenogears v0.7.93-preview renderer compiler inputs."""

import argparse
import hashlib
import io
import os
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tarfile
import tempfile


UPSTREAM_COMMIT = "e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6"
UPSTREAM_TREE = "6b18f773fd586184c4e237676c480fdfae91c7d6"
# Exact lengths and digests of the corresponding-source companions distributed
# inside the two release assets, plus their restored-inputs.sha256 digests.
BUNDLES = {
    "linux": (
        714053, "36346e722abc1c8c813d608e7bc43f5898a11ce20abf33e4495bae018c2202ce",
        73, "3260a25aaf91161074e190e14639fc51239942d8184a7f83101403df18ba90e3",
    ),
    "windows": (
        724853, "f9909dd156cb78f3f7cf7643145111f4725302aa408fec7175e547b3cb7a6ee6",
        74, "84103d76d9f7e7b8052125d3d5575597b649f13320de3ca333a1ccce2428e018",
    ),
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def unpack_companion(raw, directory):
    with tarfile.open(fileobj=io.BytesIO(raw), mode="r:gz") as archive:
        for entry in archive:
            path = PurePosixPath(entry.name)
            if (not entry.isfile() or entry.issparse() or not path.parts
                    or path.is_absolute() or ".." in path.parts
                    or "\\" in entry.name or ":" in entry.name):
                raise ValueError(f"Unexpected companion member {entry.name!r}")
            destination = directory.joinpath(*path.parts)
            destination.parent.mkdir(parents=True, exist_ok=True)
            with archive.extractfile(entry) as source, destination.open("xb") as output:
                output.write(source.read())


def verify_restored(root, checksum_file, count, checksum_digest):
    raw = checksum_file.read_bytes()
    if digest(raw) != checksum_digest:
        raise ValueError("The restored input manifest has the wrong SHA-256")
    expected = {}
    for line in raw.decode("utf-8").splitlines():
        file_digest, name = line.split("  ", 1)
        expected[name] = file_digest
    actual = {p.relative_to(root).as_posix(): digest(p.read_bytes())
              for p in root.rglob("*") if p.is_file()}
    if len(expected) != count or actual != expected:
        raise ValueError("Restored paths or file digests differ from the published manifest")


def restore(args):
    size, bundle_digest, count, checksum_digest = BUNDLES[args.platform]
    with args.source_bundle.open("rb") as source:
        raw = source.read(size + 1)
    if len(raw) != size or digest(raw) != bundle_digest:
        raise ValueError(f"Expected the {args.platform} v0.7.93-preview renderer companion "
                         f"with SHA-256 {bundle_digest}")

    upstream = args.upstream.resolve(strict=True)
    git_env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
    git_env["GIT_NO_REPLACE_OBJECTS"] = "1"
    objects = subprocess.check_output(
        ["git", "-C", str(upstream), "rev-parse",
         UPSTREAM_COMMIT + "^{commit}", UPSTREAM_COMMIT + "^{tree}"],
        env=git_env, text=True,
    ).splitlines()
    if objects != [UPSTREAM_COMMIT, UPSTREAM_TREE]:
        raise ValueError("The pinned upstream PsyCross commit and tree are required")

    out = args.out.absolute()
    if os.path.lexists(out):
        raise ValueError(f"Destination already exists: {out}")
    with tempfile.TemporaryDirectory(prefix=".renderer-", dir=out.parent) as temp:
        work = Path(temp)
        companion = work / "companion"
        staged = work / "restored"
        unpack_companion(raw, companion)
        subprocess.run([sys.executable, "-I", "-B", str(companion / "restore.py"),
                        str(upstream), str(staged)], check=True, env=git_env)
        verify_restored(staged, companion / "restored-inputs.sha256", count, checksum_digest)
        # Every published input is under pc_port. Reserve the destination
        # before moving that verified subtree, preserving concurrent attempts.
        out.mkdir()
        try:
            (staged / "pc_port").rename(out / "pc_port")
        except OSError:
            if not any(out.iterdir()):
                out.rmdir()
            raise
    print(f"Verified {count} {args.platform} renderer compiler inputs in {out}")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog="Requires Python 3.9+ and Git. The source bundles are inside the Linux "
               "release at licenses/psycross-source/renderer-source-delta.tar.gz and "
               "the Windows release at sources/renderer-presentation.tar.gz.",
    )
    parser.add_argument("--platform", choices=BUNDLES, required=True,
                        help="target platform of the published renderer")
    parser.add_argument("--source-bundle", type=Path, required=True,
                        help="path to the nested renderer companion .tar.gz")
    parser.add_argument("--upstream", type=Path, required=True,
                        help=f"local OpenDriver2/PsyCross Git repository containing {UPSTREAM_COMMIT}")
    parser.add_argument("--out", type=Path, required=True,
                        help="new source directory under an existing parent")
    args = parser.parse_args()
    try:
        restore(args)
    except (OSError, ValueError, subprocess.CalledProcessError, tarfile.TarError) as error:
        parser.exit(1, f"Renderer preparation failed: {error}\n")


if __name__ == "__main__":
    main()
