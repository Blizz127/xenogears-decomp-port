#!/usr/bin/env python3
"""Extract the retail archive (0x20, 0), file 1, without decompression.

This payload is linked for 0x801E5000 and is distinct from other battle
modules that reuse that address. Retail data stays in ignored local outputs.
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
PAYLOAD_SHA256 = '64668d85bf48dea46cf6d5f04b38e38ca887877cfef53961f0c26cd2d363b670'
CONTROLLER_SHA256 = '1f11c9ac7117c0d702c6829cb3fb57806d73c6413eaec5f9fbf1ecca2b427a2e'


def extract(disc_path):
    with disc_path.open('rb') as stream:
        disc_hash = hashlib.file_digest(stream, 'sha256').hexdigest()
    if disc_hash != DISC_SHA256:
        raise ValueError(f'disc SHA256 mismatch: {disc_hash}')
    with CdromXa(str(disc_path)) as disc:
        header = disc.read_sector(0x28)
        table = b''.join(disc.read_sector(n) for n in range(0x18, 0x28))
        raw_base = int.from_bytes(header[0x40:0x42], 'little')
        # Header bases and file IDs are one-based; file 1 is the first entry.
        table_index = raw_base - 1
        offset = table_index * 7
        entry = table[offset:offset + 7]
        if raw_base != 3088 or entry != bytes.fromhex('7fec033c4c0000'):
            raise ValueError('archive 0x20/file 1 directory or entry mismatch')
        sector = int.from_bytes(entry[:3], 'little')
        size = int.from_bytes(entry[3:], 'little')
        data = b''.join(disc.read_sector(sector + n)
                        for n in range((size + 2047) // 2048))[:size]
    payload_hash = hashlib.sha256(data).hexdigest()
    controller_hash = hashlib.sha256(data[0x1CE8:0x21D4]).hexdigest()
    if payload_hash != PAYLOAD_SHA256 or controller_hash != CONTROLLER_SHA256:
        raise ValueError('extracted payload or controller SHA256 mismatch')
    return data, {
        'disc': str(disc_path), 'disc_sha256': disc_hash,
        'archive_directory': 0x20, 'archive_offset': 0, 'file_id': 1,
        'raw_directory_base': raw_base, 'table_index': table_index,
        'table_offset': offset, 'table_entry_hex': entry.hex(),
        'first_sector': sector, 'size': size, 'load_address': '0x801E5000',
        'compression': 'none', 'payload_sha256': payload_hash,
        'payload_sha1': hashlib.sha1(data).hexdigest(),
        'controller_range': ['0x801E6CE8', '0x801E71D4'],
        'controller_sha256': controller_hash,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc', type=Path, default=ROOT/'disc/disc1.bin')
    parser.add_argument('--output', type=Path, default=ROOT/'disc/battle_command_file1.bin')
    args = parser.parse_args()
    data, metadata = extract(args.disc)
    if args.output.exists():
        if args.output.read_bytes() != data:
            raise ValueError(f'refusing to overwrite differing output: {args.output}')
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        fd, temporary = tempfile.mkstemp(prefix=args.output.name + '.', dir=args.output.parent)
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
