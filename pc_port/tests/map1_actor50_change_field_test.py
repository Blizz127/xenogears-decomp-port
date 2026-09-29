#!/usr/bin/env python3
"""Disc-backed decode of map 1 actor 50's mountain CHANGE_FIELD.

Drives the same archive + LZSS path FieldLoad uses (directory 4, map-file
slot 0xBA, scripts section at header +0x144) and asserts the leave stream
bytes. Fails if a story-var ConditionalJmp appears between FE54 and
CHANGE_FIELD, or if the CHANGE_FIELD target is not map 15 / entrance 1.
"""
from __future__ import annotations

import hashlib
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "scripts"))
from psx.scan_field_anim_opcodes import DiscArchive, lzss_decompress, u32  # noqa: E402


def require(cond: bool, msg: str) -> None:
    if not cond:
        print(f"MAP1_ACTOR50 FAIL {msg}")
        raise SystemExit(1)


def main() -> int:
    disc = ROOT / "disc" / "disc1.bin"
    require(disc.is_file(), f"missing {disc}")
    archive = DiscArchive(disc)
    blob = archive.read_entry(0xB8 + 1 * 2)
    archive.close()
    require(len(blob) >= 0x154, f"map blob too small {len(blob)}")
    scripts_off = u32(blob, 0x144)
    scripts = lzss_decompress(blob[scripts_off:])
    num = u32(scripts, 0x80)
    base = 0x84 + (num << 6)
    require(num == 64, f"numScripts={num} want 64")
    require(base == 0x1084, f"script_base=0x{base:x} want 0x1084")
    table = scripts[0x84 + 50 * 0x40 : 0x84 + 51 * 0x40]
    r0, r1 = struct.unpack_from("<HH", table, 0)
    require(r0 == 0x1806 and r1 == 0x1808, f"r0={r0:04x} r1={r1:04x}")
    data = scripts[base:]
    # Zone 5/2/3 all call 0x182A; 7/1/0/4/6 call 0x1842.
    require(data[0x1808:0x180C] == bytes([0x0A, 0x05, 0x2A, 0x18]), "zone5")
    require(data[0x180C:0x1810] == bytes([0x0A, 0x02, 0x2A, 0x18]), "zone2")
    require(data[0x1824:0x1828] == bytes([0x0A, 0x03, 0x2A, 0x18]), "zone3")
    require(data[0x1810:0x1814] == bytes([0x0A, 0x07, 0x42, 0x18]), "zone7-not-exit")
    require(data[0x1833:0x1835] == bytes([0xFE, 0x54]), "FE54 before CHANGE_FIELD")
    require(data[0x1835:0x183B] == bytes([0xFE, 0x0E, 0x00, 0x80, 0x78, 0x80]), "FE0E only")
    require(data[0x183B:0x1840] == bytes([0x98, 0x0F, 0x80, 0x01, 0x80]), "CHANGE_FIELD 15/1")
    gap = data[0x1835:0x183B]
    require(gap[0] == 0xFE and gap[1] == 0x0E, "gap is not FE0E")
    require(0x02 not in gap, "story ConditionalJmp in FE54..CHANGE_FIELD gap")
    digest = hashlib.sha256(scripts).hexdigest()
    print(f"MAP1_ACTOR50 PASS scripts_sha256={digest}")
    print("MAP1_ACTOR50 PASS CHANGE_FIELD map=15 entrance=1")
    print("MAP1_ACTOR50 PASS no story-flag opcode between FE54 and CHANGE_FIELD")
    print("MAP1_ACTOR50 PASS mountain exit live on day (zones 5/2/3); not night-gated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
