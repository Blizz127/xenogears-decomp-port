#!/usr/bin/env python3
"""Extract the retail battle RESULTS module, archive (0x10, 0), file 4.

The payload is linked for 0x801DE000 and loaded there uncompressed by the
battle; the battle intro module and other overlays reuse that range.  This
reads ./disc/disc1.bin only and writes the payload where it is told -- a test's
artifact directory, never the tree.  Retail data is never committed; the
digests below are identity constants.

Sector math follows OverlayExtractor (tools/scripts/psx/overlay.py) and
extract_battle_command_file1.py: header bases and file ids are one-based, so
table entry = raw_base(0x10) + file 4 - 2.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile

from cdrom.cdxa import CdromXa

ROOT = Path(__file__).resolve().parents[2]
DISC_SHA256 = '39c547a9afc6da15d847ef81a2c6cea1a6516bdfa562cf13b0999b04e8598bda'
LOAD_ADDRESS = 0x801DE000
SIZE = 0x6500
# Immutable code/rodata 0x801DE000..0x801E44C0; matches RESULTS_IMMUTABLE_BYTES
# and RESULTS_IMMUTABLE_SHA256 in pc_port/src/battle_results_gate.{h,inc}.
IMMUTABLE_BYTES = 0x64C0
PAYLOAD_SHA256 = 'f474fd482506b89eaff9601d5958a1c867d35a4ad11c1f58f0ecef630008a0c8'
IMMUTABLE_SHA256 = '80c31ea1289a0ce11e2440ee9759494503347c5a27cbb325d33a4492e4d74f67'
RAW_DIRECTORY_BASE = 2594
TABLE_ENTRY_HEX = 'daa60300650000'
# Entry routine 0x801E252C (called from the battle at 0x80071690) opens with
# ADDIU SP,SP,-0x20.
ENTRY_OFFSET = 0x452C
ENTRY_WORD = 0x27BDFFE0


def extract(disc_path):
    with disc_path.open('rb') as stream:
        disc_hash = hashlib.file_digest(stream, 'sha256').hexdigest()
    if disc_hash != DISC_SHA256:
        raise ValueError(f'disc SHA256 mismatch: {disc_hash}')
    with CdromXa(str(disc_path)) as disc:
        header = disc.read_sector(0x28)
        table = b''.join(disc.read_sector(n) for n in range(0x18, 0x28))
        raw_base = int.from_bytes(header[0x20:0x22], 'little')
        table_index = raw_base + 4 - 2
        offset = table_index * 7
        entry = table[offset:offset + 7]
        if raw_base != RAW_DIRECTORY_BASE or entry != bytes.fromhex(TABLE_ENTRY_HEX):
            raise ValueError('archive 0x10/file 4 directory or entry mismatch')
        sector = int.from_bytes(entry[:3], 'little')
        size = int.from_bytes(entry[3:], 'little')
        data = b''.join(disc.read_sector(sector + n)
                        for n in range((size + 2047) // 2048))[:size]
    payload_hash = hashlib.sha256(data).hexdigest()
    immutable_hash = hashlib.sha256(data[:IMMUTABLE_BYTES]).hexdigest()
    entry_word = int.from_bytes(data[ENTRY_OFFSET:ENTRY_OFFSET + 4], 'little')
    if (size != SIZE or payload_hash != PAYLOAD_SHA256 or
            immutable_hash != IMMUTABLE_SHA256 or entry_word != ENTRY_WORD):
        raise ValueError('extracted results module size/SHA256/entry mismatch')
    return data, {
        'disc': str(disc_path), 'disc_sha256': disc_hash,
        'archive_directory': 0x10, 'archive_offset': 0, 'file_id': 4,
        'raw_directory_base': raw_base, 'table_index': table_index,
        'table_offset': offset, 'table_entry_hex': entry.hex(),
        'first_sector': sector, 'size': size,
        'load_address': f'0x{LOAD_ADDRESS:08X}', 'compression': 'none',
        'payload_sha256': payload_hash,
        'immutable_range': [f'0x{LOAD_ADDRESS:08X}',
                            f'0x{LOAD_ADDRESS + IMMUTABLE_BYTES:08X}'],
        'immutable_sha256': immutable_hash,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc', type=Path, default=ROOT / 'disc/disc1.bin')
    parser.add_argument('--output', type=Path, required=True,
                        help='destination file (a test artifact directory)')
    args = parser.parse_args()
    data, metadata = extract(args.disc)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=args.output.name + '.',
                                     dir=args.output.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
        os.replace(temporary, args.output)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    metadata['output'] = str(args.output)
    print(json.dumps(metadata, indent=2))


if __name__ == '__main__':
    main()
