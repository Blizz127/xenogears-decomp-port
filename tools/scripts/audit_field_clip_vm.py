#!/usr/bin/env python3
"""Inventory pinned retail clip-VM addresses. Never certify implementation.

Reads user-owned disc sectors, emits metadata only, and does not change the
disc, game state, sources, or live process. Direct call sites are a lexical
instruction census, NOT a control-flow/dependency-closure proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

BASE = 0x801DC000
VM_START = 0x801E39F0
VM_END = 0x801E59D4
OVERLAY_SHA = "14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523"
VM_SHA = "e6e99115c22e250c6fb388ac09e484537fea5412a2b5f7852639df03354900c5"
TABLE_SHA = "af846cb60b1b8b9f6959042945ebabc40c84f0c189789b34948e3a28ee409e2c"


def read_overlay(path):
    payload = bytearray()
    with Path(path).open("rb") as disc:
        for sector in range(231361, 231386):
            disc.seek(sector * 2352 + 24)
            block = disc.read(2048)
            if len(block) != 2048:
                raise ValueError(f"short disc read at sector {sector}")
            payload.extend(block)
    return bytes(payload)


def require_hash(data, expected, label):
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise ValueError(f"{label} SHA mismatch: {actual}")


def inventory(payload):
    if len(payload) != 25 * 2048:
        raise ValueError("overlay size mismatch")
    require_hash(payload, OVERLAY_SHA, "overlay")
    body = payload[VM_START - BASE:VM_END - BASE]
    table = payload[0x40:0x204]
    require_hash(body, VM_SHA, "VM")
    require_hash(table, TABLE_SHA, "dispatch table")
    require_hash(payload[0x9CD8:0x9D44],
                 "e1810ab5a08ecf8609402635dbd2c7a0c00f24ce0b4a11263a6685beff1ca198",
                 "sound-bank selector")
    dispatch = []
    for opcode, (target,) in enumerate(struct.iter_unpack("<I", table)):
        if target & 3 or not VM_START <= target < VM_END:
            raise ValueError(f"invalid dispatch target for opcode {opcode:02x}")
        dispatch.append({"opcode": opcode, "entry": f"0x{target:08x}"})
    calls = []
    indirect_calls = []
    for offset, (word,) in enumerate(struct.iter_unpack("<I", body)):
        pc = VM_START + offset * 4
        if word >> 26 == 3:  # MIPS JAL, region from PC+4.
            target = ((pc + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
            calls.append({"site": f"0x{pc:08x}", "target": f"0x{target:08x}"})
        elif word >> 26 == 0 and word & 63 == 9:  # JALR.
            indirect_calls.append(f"0x{pc:08x}")
    return {
        "scope": "retail byte inventory only; no native parity or gameplay proof",
        "overlay_sha256": OVERLAY_SHA,
        "vm_sha256": VM_SHA,
        "dispatch_sha256": TABLE_SHA,
        "instruction_bytes": len(body),
        "unique_dispatch_entries": len({e["entry"] for e in dispatch}),
        "continuation_opcodes": [e["opcode"] for e in dispatch if e["entry"] == "0x801e5974"],
        "dispatch": dispatch,
        "direct_calls": calls,
        "indirect_call_sites": indirect_calls,
        # Manually traced from the pinned handler and callee instructions.
        # This is an address/data-flow record, not a claim of native closure.
        "sound_bank_route": {
            "opcode": 0x3C,
            "handler": "0x801e4f00",
            "bank_call": "0x801e4f18",
            "bank_helper": "0x801e5cd8",
            "consumer_call": "0x801e4f28",
            "consumer": "0x8003a3b8",
            "selector_source": "instruction high byte (s5), low8",
            "selector_zero_slot": "0x8005919c",
            "selector_one_object_offset": 0xB0,
            "selector_two_object_offset": 0xB4,
            "package_bank_pointer_offset": 8,
            "bank_id_halfword_offset": 0x14,
            "bank_id_shift": 16,
            "other_selector_result": 2,
            "native_global_owner": "UNRESOLVED",
        },
        "limitations": [
            "Calls are lexical, not reachability or transitive closure.",
            "Handler aliases do not imply identical behavior; opcode remains available.",
            "No opcode implementation status is inferred from a native symbol name.",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("disc", type=Path, help="retail disc1.bin (raw 2352-byte sectors)")
    args = parser.parse_args()
    try:
        print(json.dumps(inventory(read_overlay(args.disc)), indent=2))
    except (OSError, ValueError) as error:
        parser.exit(1, f"CLIP VM INVENTORY FAIL: {error}\n")


if __name__ == "__main__":
    main()
