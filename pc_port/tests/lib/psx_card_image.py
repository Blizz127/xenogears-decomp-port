"""Reader for raw 128 KiB PS1 memory-card images (.mcd/.mcr, psx-spx "Memory
Card Data Format"), used by the card save/load runtime tests to check that the
port's virtual card holds retail-format blocks.

Directory: frame 0 "MC" + XOR checksum; frames 1..15 describe data blocks
1..15 (state 0x51 first/only, 0x52 middle, 0x53 last, 0xA0 free, 0xA1..0xA3
deleted; u32 file size; u16 next block; name at +0x0A; XOR at +0x7F).
File header (first block): "SC", icon flag (0x11/0x12/0x13 = 1/2/3 frames),
block count, 64-byte Shift-JIS title, CLUT at +0x60, icon frames at +0x80.
"""
import struct

CARD_BYTES = 128 * 1024
BLOCK = 0x2000
FRAME = 0x80


def xor7f(frame):
    x = 0
    for b in frame[:0x7F]:
        x ^= b
    return x


class CardFile:
    def __init__(self, name, first, blocks, size, data):
        self.name, self.first, self.blocks, self.size, self.data = name, first, blocks, size, data

    @property
    def header(self):
        return self.data[:0x200]


def read_card(path):
    """Return (errors, files) for the card image at path."""
    img = open(path, 'rb').read()
    errors = []
    if len(img) != CARD_BYTES:
        return ['size %d != %d' % (len(img), CARD_BYTES)], []
    if img[0:2] != b'MC':
        errors.append('frame 0 lacks "MC"')
    for f in range(0, 16):
        frame = img[f * FRAME:(f + 1) * FRAME]
        if xor7f(frame) != frame[0x7F]:
            errors.append('directory frame %d checksum' % f)
    files = []
    for blk in range(1, 16):
        e = img[blk * FRAME:(blk + 1) * FRAME]
        state, size, nxt = struct.unpack_from('<IIH', e, 0)
        if state != 0x51:
            continue
        name = e[0x0A:0x0A + 21].split(b'\0')[0].decode('latin1')
        chain, b, data = [blk], blk, b''
        while True:
            data += img[b * BLOCK:(b + 1) * BLOCK]
            n = struct.unpack_from('<H', img, b * FRAME + 8)[0]
            if n == 0xFFFF:
                break
            b = n + 1
            if b in chain or not 1 <= b <= 15:
                errors.append('%s: bad chain' % name)
                break
            chain.append(b)
        if size != len(chain) * BLOCK:
            errors.append('%s: size %d for %d blocks' % (name, size, len(chain)))
        files.append(CardFile(name, blk, len(chain), size, data))
    return errors, files


def check_save_file(f):
    """Retail-format checks for one save file; returns a list of problems."""
    p = []
    h = f.header
    if h[0:2] != b'SC':
        p.append('%s: header magic %r' % (f.name, h[0:2]))
    if h[2] not in (0x11, 0x12, 0x13):
        p.append('%s: icon flag 0x%02x' % (f.name, h[2]))
    if h[3] != f.blocks:
        p.append('%s: header blocks %d != %d' % (f.name, h[3], f.blocks))
    if not any(h[4:0x44]):
        p.append('%s: empty title' % f.name)
    return p


def xenogears_payload_ok(f):
    """func_801CBD90 writes the 0x100-byte header, then the payload with the
    byte sum of file[0x100..0x1FFE] stored at file[0x1FFF]; func_801CB304
    checks exactly that on load."""
    d = f.data
    return (sum(d[0x100:0x1FFF]) & 0xFF) == d[0x1FFF]
