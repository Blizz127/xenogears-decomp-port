#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Prepare the seven local data files used by the r7.21 Xenogears PC port."""
import argparse
import hashlib
import os
from pathlib import Path
import sys
import tempfile

# Verified addresses for this particular pressing, in MODE2/2352 sectors.
# Each entry identifies (first sector, encoded bytes, decoded bytes, SHA-256).
IMAGE_SIZE = 718738272
IMAGE_SHA1 = "12db8ccb93516c391630f046a143762337cc21f4"
FILES = {
    "SLUS_006.64": (108606, 303104, 303104, "dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"),
    "field.bin": (108933, 125307, 260862, "38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"),
    "world_map.bin": (108995, 92180, 180422, "4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"),
    "menu.bin": (239335, 153864, 153864, "82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d"),
    "member_change_menu.bin": (239411, 26624, 26624, "3b9e2b890c27ae0de97fe343ac75cd35c05fe7f9605ae387d2166da0be78109c"),
    "shop_menu.bin": (239515, 55296, 55296, "7890e14bcabddcf85368de10783ecff8daa166dc5ac254e06db5e27959cab4cf"),
}


def check_image(stream, copy=None):
    if os.fstat(stream.fileno()).st_size != IMAGE_SIZE:
        raise ValueError("Expected a single-track 718738272-byte raw US Disc 1 BIN.")
    digest = hashlib.sha1()
    for block in iter(lambda: stream.read(1024 * 1024), b""):
        digest.update(block)
        if copy is not None:
            copy.write(block)
    if digest.hexdigest() != IMAGE_SHA1:
        raise ValueError("Unsupported image. Expected US Disc 1 SHA-1 " + IMAGE_SHA1)


def read_extent(stream, sector, count):
    stream.seek(sector * 2352)
    data = bytearray()
    while len(data) < count:
        raw = stream.read(2352)
        if len(raw) != 2352 or raw[15] != 2:
            raise ValueError("Invalid Mode 2 sector")
        data.extend(raw[24:2072])
    return data[:count]


def expand(block, size):
    if len(block) < 4 or int.from_bytes(block[:4], "little") != size:
        raise ValueError("Invalid overlay header")
    result = bytearray()
    cursor = 4
    while cursor < len(block):
        flags = block[cursor]
        cursor += 1
        for bit in range(8):
            if flags & (1 << bit):
                if cursor + 2 > len(block):
                    raise ValueError("Truncated overlay copy")
                lo, hi = block[cursor:cursor + 2]
                cursor += 2
                back = ((hi & 15) << 8) | lo
                length = (hi >> 4) + 3
                if not 1 <= back <= len(result) or len(result) + length > size:
                    raise ValueError("Invalid overlay copy")
                for _ in range(length):
                    result.append(result[-back])
            else:
                if cursor >= len(block):
                    raise ValueError("Truncated overlay literal")
                result.append(block[cursor])
                cursor += 1
            if len(result) == size:
                if cursor != len(block):
                    raise ValueError("Unexpected bytes after overlay")
                return result
    raise ValueError("Incomplete overlay")


def verify_existing(target):
    with (target / "disc1.bin").open("rb") as stream:
        check_image(stream)
    for name, (_, _, size, digest) in FILES.items():
        path = target / name
        if path.stat().st_size != size or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise ValueError(f"Existing {name} differs. Choose a new output directory.")
    print("Existing data directory verified:", target)


def prepare(image, destination):
    target = destination.absolute()
    if target.is_symlink():
        raise ValueError("Choose a new directory or an existing prepared directory.")
    if target.exists():
        with image.open("rb") as stream:
            check_image(stream)
        verify_existing(target)
        return
    if not target.parent.is_dir():
        raise ValueError("Create the parent directory first.")
    with tempfile.TemporaryDirectory(prefix=".xenogears-prep-", dir=target.parent) as temp:
        working = Path(temp) / "data"
        working.mkdir()
        with image.open("rb") as source, (working / "disc1.bin").open("xb") as copied:
            check_image(source, copied)
        with (working / "disc1.bin").open("rb") as stream:
            for name, (sector, stored, size, digest) in FILES.items():
                data = read_extent(stream, sector, stored)
                if stored != size:
                    data = expand(data, size)
                if hashlib.sha256(data).hexdigest() != digest:
                    raise ValueError("Incorrect extracted SHA-256 for " + name)
                (working / name).write_bytes(data)
        if target.exists() or target.is_symlink():
            raise ValueError("Destination appeared during preparation. Run again to verify it.")
        working.rename(target)
    print("Verified Disc 1 and six extracted files:", target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path, help="your legally owned US Disc 1 raw BIN")
    parser.add_argument("directory", type=Path, help="new output directory, or a prepared directory to verify")
    args = parser.parse_args()
    try:
        prepare(args.image, args.directory)
    except (OSError, ValueError) as exc:
        parser.exit(1, f"Data preparation failed: {exc}\n")


if __name__ == "__main__":
    main()
