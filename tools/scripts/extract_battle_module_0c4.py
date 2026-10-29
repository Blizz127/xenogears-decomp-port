#!/usr/bin/env python3
"""Extract the retail archive 0x0C, file 4, without decompression.

This payload is the battle startup module linked for 0x801E4000 (stage
loader, dispatcher, timer task and startup actor/resource/queue routines).
It is distinct from the other modules that reuse 0x801E4000.. addresses
(battle_command_file1 at 0x801E5000, menu.bin, the field object overlay).
Retail data stays in ignored local outputs.
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
PAYLOAD_SHA256 = '4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885'
LOAD_ADDRESS = 0x801E4000
# The windows the port's module hosts pin before adopting any body
# (battle_startup_actors_host.inc and friends).
IDENTITY_SHA256 = {
    0x801E4000: (32, '6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140'),
    0x801E5840: (228, 'e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2'),
}


def extract(disc_path):
    with disc_path.open('rb') as stream:
        disc_hash = hashlib.file_digest(stream, 'sha256').hexdigest()
    if disc_hash != DISC_SHA256:
        raise ValueError(f'disc SHA256 mismatch: {disc_hash}')
    with CdromXa(str(disc_path)) as disc:
        header = disc.read_sector(0x28)
        table = b''.join(disc.read_sector(n) for n in range(0x18, 0x28))
        raw_base = int.from_bytes(header[0x0C * 2:0x0C * 2 + 2], 'little')
        # Header bases and file IDs are one-based; file 4 is the fourth entry.
        table_index = raw_base - 1 + 3
        offset = table_index * 7
        entry = table[offset:offset + 7]
        if raw_base != 2613 or entry != bytes.fromhex('a1a803c0560000'):
            raise ValueError('archive 0x0C/file 4 directory or entry mismatch')
        sector = int.from_bytes(entry[:3], 'little')
        size = int.from_bytes(entry[3:], 'little')
        data = b''.join(disc.read_sector(sector + n)
                        for n in range((size + 2047) // 2048))[:size]
    payload_hash = hashlib.sha256(data).hexdigest()
    if payload_hash != PAYLOAD_SHA256:
        raise ValueError('extracted payload SHA256 mismatch')
    for address, (length, expected) in IDENTITY_SHA256.items():
        start = address - LOAD_ADDRESS
        if hashlib.sha256(data[start:start + length]).hexdigest() != expected:
            raise ValueError(f'identity window 0x{address:08X} mismatch')
    return data, {
        'disc': str(disc_path), 'disc_sha256': disc_hash,
        'archive_directory': 0x0C, 'file_id': 4,
        'raw_directory_base': raw_base, 'table_index': table_index,
        'table_offset': offset, 'table_entry_hex': entry.hex(),
        'first_sector': sector, 'size': size, 'load_address': f'0x{LOAD_ADDRESS:08X}',
        'compression': 'none', 'payload_sha256': payload_hash,
        'payload_sha1': hashlib.sha1(data).hexdigest(),
        'identity_windows': [f'0x{a:08X}+{n}' for a, (n, _) in IDENTITY_SHA256.items()],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc', type=Path, default=ROOT/'disc/disc1.bin')
    parser.add_argument('--output', type=Path, default=ROOT/'disc/battle_module_0c4.bin')
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
