#!/usr/bin/env python3
"""Pin the Lahan painting-room fire cue to the retail Disc 1 scripts."""

from __future__ import annotations

import hashlib
import runpy
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCAN = runpy.run_path(
    str(ROOT / "tools/scripts/psx/scan_field_anim_opcodes.py"),
    run_name="field_archive_library",
)

EXPECTED_SCRIPT_SHA256 = {
    13: "c8742129dae5fff7c6615903fe64b3742ce6509d6aba0c5a86a31114bb3771fe",
    14: "8fbd723620fbd6b3b8ff48b0d72d4347dcbd0a6492fef525eb89221cec08631b",
    15: "dfba23f3e90c0dd9877d3fa4237b4b8a12d1fa7a3cd153a34bf52984acf62327",
}


def scripts_for_map(archive: object, map_id: int) -> tuple[bytes, int]:
    map_file = archive.read_entry(SCAN["FIELD_MAP_FIRST_ENTRY"] + map_id * 2)
    compressed_offset = struct.unpack_from("<I", map_file, 0x144)[0]
    scripts = SCAN["lzss_decompress"](map_file[compressed_offset:])
    digest = hashlib.sha256(scripts).hexdigest()
    assert digest == EXPECTED_SCRIPT_SHA256[map_id], (
        f"map {map_id} script source changed: {digest}"
    )
    num_scripts = struct.unpack_from("<I", scripts, 0x80)[0]
    return scripts, 0x84 + num_scripts * 0x40


def find_extended_opcode(scripts: bytes, bytecode_base: int, opcode: int) -> list[int]:
    marker = bytes((0xFE, opcode))
    return [
        offset - bytecode_base
        for offset in range(bytecode_base, len(scripts) - 1)
        if scripts[offset : offset + 2] == marker
    ]


def main() -> None:
    archive = SCAN["DiscArchive"](ROOT / "disc/disc1.bin")
    try:
        scripts13, base13 = scripts_for_map(archive, 13)
        scripts14, base14 = scripts_for_map(archive, 14)
        scripts15, base15 = scripts_for_map(archive, 15)
    finally:
        archive.close()

    # Pin the entry table independently of the byte-pattern scan below.
    # This test does not execute routines or prove cue reachability/lifetime.
    actor1_routine0, actor1_routine1 = struct.unpack_from("<2H", scripts14, 0x84 + 0x40)
    assert actor1_routine0 == 0x8E
    assert actor1_routine1 == 0xB5
    assert find_extended_opcode(scripts14, base14, 0x66) == [0xBA]
    assert scripts14[base14 + 0xBA : base14 + 0xC4] == bytes.fromhex(
        "fe 66 36 80 40 80 20 80 03 80"
    )

    # Raw marker absence only: not a full control-flow or audio-path audit.
    # In particular this does not prove audible silence after room exit.
    assert find_extended_opcode(scripts13, base13, 0x66) == []
    assert find_extended_opcode(scripts15, base15, 0x66) == []
    assert find_extended_opcode(scripts14, base14, 0x8D) == []

    print("FIELD FIRE CUE RETAIL SOURCE PASS map14_ip=0x00ba cue=0x36 channel=3")


if __name__ == "__main__":
    main()
