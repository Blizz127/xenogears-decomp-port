#!/usr/bin/env python3
"""
Extract a plain ISO9660 file (by default the PSX boot executable SLUS_006.64)
from a raw CD-XA .bin image (2352-byte Mode2/Form1 sectors).

This reuses the project's existing CD reader (cdrom.cdxa.CdromXa) and ISO9660
structures (cdrom.fs) so the Mode2/Form1 sector handling matches what the
overlay extractor already relies on.

Unlike tools/scripts/extract_overlays.py (which reads Xenogears' custom archive
table), the main executable lives in the standard ISO9660 filesystem, so we walk
the root directory and copy the file's data sectors out verbatim.
"""

import argparse
import math
import os
import sys

# Allow running from any cwd: make sibling packages (cdrom/, psx/) importable.
sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))

from cdrom.cdxa import CdromXa
from cdrom.fs import VolumeDescriptor, DirectoryRecord

# Logical block where the ISO9660 Primary Volume Descriptor lives.
PVD_SECTOR = 16


def find_disc_dir():
    start = os.path.abspath(os.path.dirname(__file__))
    candidate = f"{start}/disc"
    while start != "/":
        if os.path.isdir(candidate):
            return candidate
        start = os.path.dirname(start)
        candidate = f"{start}/disc"
    print("WARNING: failed to find disc dir, using current working directory instead")
    return os.getcwd()


def read_directory(disc: CdromXa, start_lba: int, size: int) -> bytes:
    """Read `size` bytes of a directory extent starting at `start_lba`."""
    sector_count = math.ceil(size / CdromXa.SECTOR_DATA_SIZE)
    data = b""
    for i in range(sector_count):
        data += disc.read_sector(start_lba + i)
    return data[:size]


def iter_directory_records(directory: bytes):
    """Yield (start_lba, data_size, name) for each record in a directory extent.

    Directory records never span a 2048-byte logical sector; a record length of
    0 means "skip to the next sector boundary".
    """
    pos = 0
    total = len(directory)
    while pos < total:
        record_len = directory[pos]
        if record_len == 0:
            # Jump to the next logical-sector boundary.
            next_boundary = (
                (pos // CdromXa.SECTOR_DATA_SIZE) + 1
            ) * CdromXa.SECTOR_DATA_SIZE
            if next_boundary <= pos:
                break
            pos = next_boundary
            continue

        record = DirectoryRecord(directory[pos : pos + record_len])
        yield (record.data_logical_block_num, record.data_size, record.name)
        pos += record_len


def names_match(record_name: bytes, target: str) -> bool:
    decoded = record_name.decode("ascii", errors="replace")
    # ISO9660 file identifiers usually carry a ";1" version suffix.
    stripped = decoded.split(";", 1)[0]
    return decoded.upper() == target.upper() or stripped.upper() == target.upper()


def extract_file(disc: CdromXa, target_name: str, output_path: str):
    pvd = VolumeDescriptor(disc.read_sector(PVD_SECTOR))
    root = pvd.root_dir_record
    directory = read_directory(disc, root.data_logical_block_num, root.data_size)

    for start_lba, data_size, name in iter_directory_records(directory):
        if names_match(name, target_name):
            print(
                f"Found {name!r} at LBA {start_lba} (size {data_size} bytes)"
            )
            with open(output_path, "wb") as fh:
                disc.extract_sectors(start_lba, data_size, fh)
            # extract_sectors writes whole sectors; trim to the real file size.
            with open(output_path, "r+b") as fh:
                fh.truncate(data_size)
            print(f"Wrote {output_path}")
            return True

    return False


def parse_arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bin_file", help="Path to the raw .bin (2352-byte sectors)")
    parser.add_argument(
        "--name",
        default="SLUS_006.64",
        help="ISO9660 file identifier to extract (default: SLUS_006.64)",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Output path (default: <disc dir>/<name>)",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List root-directory files instead of extracting",
    )
    return parser.parse_args()


def main():
    args = parse_arguments()
    output_path = args.output or os.path.join(find_disc_dir(), args.name)

    with CdromXa(args.bin_file) as disc:
        if args.list:
            pvd = VolumeDescriptor(disc.read_sector(PVD_SECTOR))
            root = pvd.root_dir_record
            directory = read_directory(
                disc, root.data_logical_block_num, root.data_size
            )
            print("Root directory contents:")
            for start_lba, data_size, name in iter_directory_records(directory):
                print(f"  {name!r:40} LBA={start_lba:<8} size={data_size}")
            return

        if not extract_file(disc, args.name, output_path):
            print(
                f"ERROR: could not find {args.name!r} in the ISO9660 root directory.",
                file=sys.stderr,
            )
            sys.exit(1)


if __name__ == "__main__":
    main()
