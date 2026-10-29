#!/usr/bin/env python3
"""TEST TOOLING (read-only on the disc image; writes nothing): list the movie
requests in retail's battle event scripts, for run_battle_movie_request_test.sh.

Archive (0x20) file 2 holds the per-encounter battle event scripts:
battle_command_file1 xeno_battle_command_file1_init_5160 (before_controller.s
801E5208..801E52BC) LZSS-unpacks entry pair D_8006F9DF (u32 count, then
offsets) and sets the script base to blob + 0x44 + 16 * u32 blob[0x40].  The
after-controller dispatcher func_801E879C sends opcode 0x27 (jtbl 801E5030
entry 39) to func_801E77E4, whose func_801E57F8(cmd, 4, 0, 1) takes four u16
operands (bit 15: immediate) into ctx+0x380 type, +0x382 movie number,
+0x384 no-skip flag, +0x386 end frame.  Listed: 0x27 with four immediates,
type 0..2, no-skip 0/1 and a movie number below 0x40.

usage: find_battle_movie_requests.py DISC1_BIN
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools', 'scripts'))
from cdrom.cdxa import CdromXa  # noqa: E402


def lzss(buf):
    size = struct.unpack_from('<I', buf, 0)[0]
    out = bytearray()
    i = 4
    while len(out) < size and i < len(buf):
        control = buf[i]
        i += 1
        for bit in range(8):
            if i >= len(buf) or len(out) >= size:
                break
            if not (control >> bit) & 1:
                out.append(buf[i])
                i += 1
                continue
            low, high = buf[i], buf[i + 1]
            i += 2
            start = len(out) - (((high & 0xF) << 8) | low)
            for k in range((high >> 4) + 3):
                out.append(out[start + k])
    return bytes(out)


def main():
    with CdromXa(sys.argv[1]) as disc:
        header = disc.read_sector(0x28)
        table = b''.join(disc.read_sector(n) for n in range(0x18, 0x28))
        base = int.from_bytes(header[0x40:0x42], 'little')   # directory 0x20
        entry = table[7 * base:7 * base + 7]                   # its file 2
        sector = int.from_bytes(entry[:3], 'little')
        size = int.from_bytes(entry[3:], 'little')
        data = b''.join(disc.read_sector(sector + k) for k in range((size + 2047) // 2048))[:size]
    count = struct.unpack_from('<I', data, 0)[0]
    offsets = struct.unpack_from('<%dI' % count, data, 4) + (size,)
    print('archive 0x20 file 2: sector %#x size %#x, %d encounters' % (sector, size, count // 2))
    found = 0
    for encounter in range(count // 2):
        blob = lzss(data[offsets[2 * encounter]:offsets[2 * encounter + 1]])
        if len(blob) < 0x44:
            continue
        script = blob[0x44 + 16 * struct.unpack_from('<I', blob, 0x40)[0]:]
        for m in re.finditer(rb'\x27(..)(..)(..)(..)', script, re.S):
            v = [struct.unpack('<H', g)[0] for g in m.groups()]
            if (all(x & 0x8000 for x in v) and (v[0] & 0x7FFF) <= 2 and (v[2] & 0x7FFF) <= 1
                    and (v[1] & 0x7FFF) < 0x40):
                found += 1
                print('encounter %d (D_8006F9DF) script+%#06x: type=%d number=%d noskip=%d end=%d'
                      % (encounter, m.start(), v[0] & 0x7FFF, v[1] & 0x7FFF, v[2] & 0x7FFF,
                         v[3] & 0x7FFF))
    print('%d movie request(s)' % found)
    return 0 if found else 1


if __name__ == '__main__':
    sys.exit(main())
