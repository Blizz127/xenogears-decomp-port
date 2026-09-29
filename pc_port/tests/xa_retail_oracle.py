#!/usr/bin/env python3
"""Independent PS1 XA decoder + 37.8 -> 44.1 kHz zigzag test oracle.

Reads two pinned XA sectors from the operator-owned Disc 1 image and emits
interleaved signed 16-bit PCM. No retail payload is stored in the repository.
"""

import hashlib
import struct
import sys

SECTOR_SIZE = 2352
LBAS = (48, 56)
SECTOR_SHA256 = {
    48: "dbf14011282e3515689fd179c28c3df44a219e37c44957699e4a5019e81d6514",
    56: "bfc8932682c88fb3d9b209c575d89d7ee5a415cfdf0c139e1a769893560a7547",
}
PCM_SHA256 = "a9d1f2db85edf4148bad8d6613fb1a1d9f10906a9c6056dfeb495662cb33cf8b"
F0 = (0, 60, 115, 98)
F1 = (0, 0, -52, -55)
ZIGZAG = (
    (0, 0x0000, 0x0000, 0x0000, 0x0000, -0x0002, 0x000A, -0x0022, 0x0041, -0x0054,
     0x0034, 0x0009, -0x010A, 0x0400, -0x0A78, 0x234C, 0x6794, -0x1780, 0x0BCD, -0x0623,
     0x0350, -0x016D, 0x006B, 0x000A, -0x0010, 0x0011, -0x0008, 0x0003, -0x0001),
    (0, 0x0000, 0x0000, -0x0002, 0x0000, 0x0003, -0x0013, 0x003C, -0x004B, 0x00A2,
     -0x00E3, 0x0132, -0x0043, -0x0267, 0x0C9D, 0x74BB, -0x11B4, 0x09B8, -0x05BF, 0x0372,
     -0x01A8, 0x00A6, -0x001B, 0x0005, 0x0006, -0x0008, 0x0003, -0x0001, 0x0000),
    (0, 0x0000, -0x0001, 0x0003, -0x0002, -0x0005, 0x001F, -0x004A, 0x00B3, -0x0192,
     0x02B1, -0x039E, 0x04F8, -0x05A6, 0x7939, -0x05A6, 0x04F8, -0x039E, 0x02B1, -0x0192,
     0x00B3, -0x004A, 0x001F, -0x0005, -0x0002, 0x0003, -0x0001, 0x0000, 0x0000),
    (0, -0x0001, 0x0003, -0x0008, 0x0006, 0x0005, -0x001B, 0x00A6, -0x01A8, 0x0372,
     -0x05BF, 0x09B8, -0x11B4, 0x74BB, 0x0C9D, -0x0267, -0x0043, 0x0132, -0x00E3, 0x00A2,
     -0x004B, 0x003C, -0x0013, 0x0003, 0x0000, -0x0002, 0x0000, 0x0000, 0x0000),
    (-0x0001, 0x0003, -0x0008, 0x0011, -0x0010, 0x000A, 0x006B, -0x016D, 0x0350, -0x0623,
     0x0BCD, -0x1780, 0x6794, 0x234C, -0x0A78, 0x0400, -0x010A, 0x0009, 0x0034, -0x0054,
     0x0041, -0x0022, 0x000A, -0x0001, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000),
    (0x0002, -0x0008, 0x0010, -0x0023, 0x002B, 0x001A, -0x00EB, 0x027B, -0x0548, 0x0AFA,
     -0x16FA, 0x53E0, 0x3C07, -0x1249, 0x080E, -0x0347, 0x015B, -0x0044, -0x0017, 0x0046,
     -0x0023, 0x0011, -0x0005, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000),
    (-0x0005, 0x0011, -0x0023, 0x0046, -0x0017, -0x0044, 0x015B, -0x0347, 0x080E, -0x1249,
     0x3C07, 0x53E0, -0x16FA, 0x0AFA, -0x0548, 0x027B, -0x00EB, 0x001A, 0x002B, -0x0023,
     0x0010, -0x0008, 0x0002, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000),
)


def clamp16(value):
    return max(-32768, min(32767, value))


def decode(payload, history):
    output = []
    for group in range(18):
        chunk = payload[group * 128:(group + 1) * 128]
        for block in range(4):
            params = (chunk[4 + block * 2], chunk[5 + block * 2])
            for channel, param in enumerate(params):
                if (param >> 4) > 3:
                    raise ValueError("invalid XA filter")
            channel_samples = ([], [])
            for sample in range(28):
                packed = chunk[16 + block + sample * 4]
                for channel, nibble in enumerate((packed & 15, packed >> 4)):
                    if nibble & 8:
                        nibble -= 16
                    param = params[channel]
                    shift = param & 15
                    if shift > 12:
                        shift = 9
                    filt = param >> 4
                    previous, older = history[channel]
                    value = clamp16((nibble << (12 - shift)) +
                                    ((previous * F0[filt] + older * F1[filt] + 32) >> 6))
                    history[channel] = [value, previous]
                    channel_samples[channel].append(value)
            output.extend(zip(channel_samples[0], channel_samples[1]))
    return output


def interpolate(ring, table, pos):
    return clamp16(sum((ring[(pos - index) & 31] * coefficient) // 32768
                       for index, coefficient in enumerate(table)))


def resample(frames, rings, state):
    output = []
    pos, sixstep = state
    for left, right in frames:
        rings[0][pos] = left
        rings[1][pos] = right
        pos = (pos + 1) & 31
        sixstep -= 1
        if sixstep == 0:
            sixstep = 6
            for table in ZIGZAG:
                output.append((interpolate(rings[0], table, pos),
                               interpolate(rings[1], table, pos)))
    state[:] = (pos, sixstep)
    return output


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: xa_retail_oracle.py DISC1.BIN OUTPUT.PCM")
    history = [[0, 0], [0, 0]]
    rings = [[0] * 32, [0] * 32]
    state = [0, 6]
    pcm_hash = hashlib.sha256()
    with open(sys.argv[1], "rb") as disc, open(sys.argv[2], "wb") as output:
        for lba in LBAS:
            disc.seek(lba * SECTOR_SIZE)
            raw = disc.read(SECTOR_SIZE)
            digest = hashlib.sha256(raw).hexdigest()
            if digest != SECTOR_SHA256[lba]:
                raise SystemExit(f"Disc 1 LBA {lba} SHA-256 mismatch: {digest}")
            if raw[15] != 2 or raw[16:20] != raw[20:24] or raw[16:20] != bytes((1, 1, 0x64, 1)):
                raise SystemExit(f"Disc 1 LBA {lba} is not the pinned XA sector")
            for left, right in resample(decode(raw[24:2328], history), rings, state):
                frame = struct.pack("<hh", left, right)
                pcm_hash.update(frame)
                output.write(frame)
    if pcm_hash.hexdigest() != PCM_SHA256:
        raise SystemExit(f"XA oracle PCM SHA-256 mismatch: {pcm_hash.hexdigest()}")


if __name__ == "__main__":
    main()
