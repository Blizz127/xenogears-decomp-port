#!/usr/bin/env python3
"""Reachability scanner for field-map sprite animation bytecode.

func_800248D4 is the sprite-animation VM, not the field-script VM.  Its inputs
live in the sprite-data section of each field-map archive.  This tool reads the
retail opcode-length blob, extracts/decompresses every valid field-map file from
the disc image, seeds traversal from each animation entry, and walks the small
amount of control flow implemented by the retail dispatcher.

Partial results from an animation that aborts validation are never included in
the authoritative reachable totals.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from collections import Counter, defaultdict, deque
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


TARGETS = (0x85, 0x8E, 0x98, 0xBE, 0xC8, 0xD4, 0xE2, 0xFA)
SECTOR_RAW_SIZE = 2352
SECTOR_DATA_OFFSET = 24
SECTOR_DATA_SIZE = 2048
ARCHIVE_TABLE_SECTOR = 0x18
ARCHIVE_TABLE_SECTORS = 0x10
ARCHIVE_HEADER_SECTOR = 0x28
ARCHIVE_HEADER_SIZE = 0x7A
FIELD_DIRECTORY = 4
FIELD_MAP_FIRST_ENTRY = 0xB8


class ScanError(Exception):
    pass


class AbsentArchiveEntry(ScanError):
    pass


@dataclass(frozen=True)
class AnimationId:
    map_id: int
    package: int
    animation: int

    def text(self) -> str:
        return f"map{self.map_id:03d}/package{self.package}/animation{self.animation}"


@dataclass
class AnimationScan:
    ident: AnimationId
    start: int
    visited: set[int]
    hits: dict[int, set[int]]
    error: str | None = None


def u16(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise ScanError(f"truncated u16 at 0x{offset:x}")
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise ScanError(f"truncated u32 at 0x{offset:x}")
    return struct.unpack_from("<I", data, offset)[0]


def s16(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise ScanError(f"truncated s16 at 0x{offset:x}")
    return struct.unpack_from("<h", data, offset)[0]


def load_retail_lengths(path: Path) -> list[int]:
    values: list[int] = []
    active = False
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip() == "dlabel D_8004FC40":
            active = True
            continue
        if not active:
            continue
        if line.startswith(".size D_8004FC40"):
            break
        marker = ".byte 0x"
        if marker in line:
            values.append(int(line.split(marker, 1)[1].split()[0], 16))
    if len(values) != 256:
        raise ScanError(f"expected 256 D_8004FC40 bytes, found {len(values)}")
    return values


def effective_lengths(retail: list[int]) -> list[int]:
    # Retail func_800248D4 handles every 0x00..0x7f frame/delay opcode as one
    # byte and never indexes D_8004FC40 for that range.  The bytes preceding
    # D_8004FCC0 are adjacent sdata, not usable low-opcode lengths.
    result = [1] * 0x80 + retail[0x80:]

    # Dedicated jump-table handlers that consume a different fixed shape than
    # the generic advance tail.  Addresses in retail func_800248D4:
    #   BE: 80024A84-80024B9C (opcode + packed u16)
    #   D4: 80024C30-80024C64 (opcode + relative s16)
    #   E2: 80024BA8-80024BE0 (opcode + relative s16)
    #   FA: 80024C04-80024C2C (opcode + condition byte + relative s16)
    result[0xBE] = 3
    result[0xD4] = 3
    result[0xE2] = 3
    result[0xFA] = 4
    if any(length <= 0 for length in result):
        bad = [f"0x{i:02x}" for i, length in enumerate(result) if length <= 0]
        raise ScanError(f"zero PC advance after retail overrides: {', '.join(bad)}")
    return result


class DiscArchive:
    def __init__(self, path: Path):
        self.path = path
        self.fp = path.open("rb")
        self.table = b"".join(
            self.sector(i)
            for i in range(ARCHIVE_TABLE_SECTOR,
                           ARCHIVE_TABLE_SECTOR + ARCHIVE_TABLE_SECTORS)
        )
        self.header = self.sector(ARCHIVE_HEADER_SECTOR)[:ARCHIVE_HEADER_SIZE]
        directory_offsets = [u16(self.header, i) for i in range(0, len(self.header), 2)]
        raw_base = directory_offsets[FIELD_DIRECTORY]
        if raw_base in (0, 0xFFFF):
            raise ScanError("field archive directory is absent")
        self.field_base = raw_base - 1
        following = [value - 1 for value in directory_offsets
                     if value not in (0, 0xFFFF) and value - 1 > self.field_base]
        if not following:
            raise ScanError("cannot derive end of field archive directory")
        self.field_end = min(following)

    def close(self) -> None:
        self.fp.close()

    def sector(self, number: int) -> bytes:
        self.fp.seek(number * SECTOR_RAW_SIZE + SECTOR_DATA_OFFSET)
        data = self.fp.read(SECTOR_DATA_SIZE)
        if len(data) != SECTOR_DATA_SIZE:
            raise ScanError(f"truncated disc sector {number}")
        return data

    def entry(self, relative_index: int) -> tuple[int, int]:
        absolute = self.field_base + relative_index - 1
        if absolute < self.field_base or absolute >= self.field_end:
            raise ScanError(f"archive entry {relative_index} outside field directory")
        offset = absolute * 7
        if offset + 7 > len(self.table):
            raise ScanError(f"archive table entry {absolute} outside loaded table")
        raw = self.table[offset:offset + 7]
        return int.from_bytes(raw[:3], "little"), int.from_bytes(raw[3:], "little")

    def read_entry(self, relative_index: int) -> bytes:
        sector, size = self.entry(relative_index)
        if sector == 0 or size == 0 or size & 0x80000000:
            raise AbsentArchiveEntry(
                f"absent archive entry (sector={sector}, size=0x{size:08x})"
            )
        if size > 16 * 1024 * 1024:
            raise ScanError(f"implausible archive entry size 0x{size:x}")
        chunks = [self.sector(i) for i in range(sector, sector + (size + 2047) // 2048)]
        return b"".join(chunks)[:size]

    def map_ids(self) -> Iterable[int]:
        # Field map/VRAM files are a contiguous pair table beginning at 0xB8.
        # The first empty map-file slot terminates that table; later entries in
        # archive directory 4 belong to other field resources and must not be
        # interpreted as map headers.
        count = self.field_end - self.field_base
        max_map = (count - FIELD_MAP_FIRST_ENTRY) // 2
        for map_id in range(max_map + 1):
            sector, size = self.entry(FIELD_MAP_FIRST_ENTRY + map_id * 2)
            if sector == 0 or size == 0 or size & 0x80000000:
                break
            yield map_id


def lzss_decompress(data: bytes) -> bytes:
    if len(data) < 5:
        raise ScanError("truncated LZSS stream")
    expected = u32(data, 0)
    if expected > 32 * 1024 * 1024:
        raise ScanError(f"implausible LZSS output size 0x{expected:x}")
    ip = 4
    out = bytearray()
    while len(out) < expected:
        if ip >= len(data):
            raise ScanError("truncated LZSS control byte")
        control = data[ip]
        ip += 1
        for _ in range(8):
            if len(out) >= expected:
                break
            if control & 1:
                if ip + 2 > len(data):
                    raise ScanError("truncated LZSS back-reference")
                low, high = data[ip], data[ip + 1]
                ip += 2
                distance = ((high & 0x0F) << 8) | low
                count = (high >> 4) + 3
                if distance == 0 or distance > len(out):
                    raise ScanError(f"invalid LZSS distance {distance} at output 0x{len(out):x}")
                for _ in range(count):
                    if len(out) >= expected:
                        break
                    out.append(out[-distance])
            else:
                if ip >= len(data):
                    raise ScanError("truncated LZSS literal")
                out.append(data[ip])
                ip += 1
            control >>= 1
    return bytes(out)


def scan_animation(
    ident: AnimationId,
    package: bytes,
    start: int,
    code_lo: int,
    code_hi: int,
    lengths: list[int],
) -> AnimationScan:
    # State includes the E2/85 call stack; treating only PC as visited would
    # merge distinct return contexts and under-walk reachable code.
    pending = deque([(start, ())])
    seen_states: set[tuple[int, tuple[int, ...]]] = set()
    visited: set[int] = set()
    hits: dict[int, set[int]] = defaultdict(set)

    def check_target(source: int, target: int) -> None:
        if target < code_lo or target >= code_hi:
            raise ScanError(
                f"branch from 0x{source:x} targets 0x{target:x}, "
                f"outside code [0x{code_lo:x},0x{code_hi:x})"
            )

    try:
        while pending:
            pc, stack = pending.popleft()
            state = (pc, stack)
            if state in seen_states:
                continue
            seen_states.add(state)
            if len(seen_states) > 100000:
                raise ScanError("state limit exceeded (likely malformed recursive control flow)")
            if pc < code_lo or pc >= code_hi:
                raise ScanError(f"fetch 0x{pc:x} outside code [0x{code_lo:x},0x{code_hi:x})")

            opcode = package[pc]
            length = lengths[opcode]
            if length <= 0:
                raise ScanError(f"zero PC advance for opcode 0x{opcode:02x} at 0x{pc:x}")
            if pc + length > code_hi:
                raise ScanError(
                    f"truncated opcode 0x{opcode:02x} at 0x{pc:x}: "
                    f"needs {length} bytes before 0x{code_hi:x}"
                )
            visited.add(pc)
            if opcode in TARGETS:
                hits[opcode].add(pc)

            # Animation switch/stop handlers.  0x85 returns through the E2
            # U24 stack; an empty stack is a valid top-level termination.
            if opcode in (0x80, 0x81, 0x82, 0x8E):
                continue
            if opcode == 0x85:
                if stack:
                    pending.append((stack[-1], stack[:-1]))
                continue

            if opcode in (0xD4, 0xE1):
                target = pc + s16(package, pc + 1)
                check_target(pc, target)
                pending.append((target, stack))
                continue

            if opcode == 0xE2:
                target = pc + s16(package, pc + 1)
                check_target(pc, target)
                if len(stack) >= 64:
                    raise ScanError(f"call depth exceeds 64 at 0x{pc:x}")
                pending.append((target, stack + (pc + length,)))
                continue

            if opcode == 0xE4:
                target = pc + s16(package, pc + 1)
                check_target(pc, target)
                pending.append((target, stack))
                pending.append((pc + length, stack))
                continue

            if opcode == 0xFA:
                target = pc + s16(package, pc + 2)
                check_target(pc, target)
                pending.append((target, stack))
                pending.append((pc + length, stack))
                continue

            pending.append((pc + length, stack))
    except ScanError as error:
        return AnimationScan(ident, start, visited, dict(hits), str(error))
    return AnimationScan(ident, start, visited, dict(hits))


def scan_package(
    map_id: int,
    package_index: int,
    package: bytes,
    lengths: list[int],
) -> tuple[list[AnimationScan], dict[int, set[int]], tuple[int, int]]:
    if len(package) < 20:
        raise ScanError("package shorter than 20-byte header")
    animations_offset = u32(package, 4)
    frames_offset = u32(package, 8)
    if not (16 <= animations_offset < frames_offset <= len(package)):
        raise ScanError(
            f"invalid animation/frame bounds 0x{animations_offset:x}/0x{frames_offset:x} "
            f"for package size 0x{len(package):x}"
        )
    flags = u16(package, animations_offset)
    animation_count = flags & 0x3F
    if animation_count == 0:
        return [], {}, (animations_offset, frames_offset)
    table_end = animations_offset + 2 + animation_count * 2
    if table_end > frames_offset:
        raise ScanError("truncated animation-offset table")

    records: list[tuple[int, int]] = []
    for animation_index in range(animation_count):
        relative = u16(package, animations_offset + 2 + animation_index * 2)
        record = animations_offset + relative
        if record < table_end or record + 6 > frames_offset:
            raise ScanError(
                f"animation {animation_index} record 0x{record:x} outside metadata/code bounds"
            )
        script_start = record + u16(package, record + 2) + 2
        if script_start < table_end or script_start >= frames_offset:
            raise ScanError(
                f"animation {animation_index} script start 0x{script_start:x} "
                f"outside package code"
            )
        records.append((animation_index, script_start))

    code_lo = min(start for _, start in records)
    scans = [
        scan_animation(
            AnimationId(map_id, package_index, animation_index),
            package, start, code_lo, frames_offset, lengths,
        )
        for animation_index, start in records
    ]

    authoritative_visited: set[int] = set()
    for scan in scans:
        if scan.error is None:
            authoritative_visited.update(scan.visited)
    raw_unreachable: dict[int, set[int]] = defaultdict(set)
    for pc in range(code_lo, frames_offset):
        value = package[pc]
        if value in TARGETS and pc not in authoritative_visited:
            raw_unreachable[value].add(pc)
    return scans, dict(raw_unreachable), (code_lo, frames_offset)


def scan_map(map_id: int, map_file: bytes, lengths: list[int]) -> dict:
    if len(map_file) < 0x190:
        raise ScanError(f"map file too short: 0x{len(map_file):x}")
    compressed_offset = u32(map_file, 0x13C)
    if compressed_offset < 0x190 or compressed_offset >= len(map_file):
        raise ScanError(f"invalid sprite-data compressed offset 0x{compressed_offset:x}")
    sprite_data = lzss_decompress(map_file[compressed_offset:])
    package_count = u32(sprite_data, 0)
    if package_count > 0x100:
        raise ScanError(f"implausible package count {package_count}")
    table_end = 4 + package_count * 4
    if table_end > len(sprite_data):
        raise ScanError("truncated sprite-package offset table")
    offsets = [u32(sprite_data, 4 + index * 4) for index in range(package_count)]
    if offsets and offsets[0] != table_end:
        raise ScanError(
            f"first package offset 0x{offsets[0]:x} != table end 0x{table_end:x}"
        )
    if offsets != sorted(offsets) or any(off < table_end or off >= len(sprite_data) for off in offsets):
        raise ScanError("invalid/non-monotonic sprite-package offsets")

    scans: list[AnimationScan] = []
    unreachable: dict[int, list[dict]] = defaultdict(list)
    package_errors: list[dict] = []
    for package_index, start in enumerate(offsets):
        end = offsets[package_index + 1] if package_index + 1 < len(offsets) else len(sprite_data)
        package = sprite_data[start:end]
        try:
            package_scans, raw, bounds = scan_package(map_id, package_index, package, lengths)
            scans.extend(package_scans)
            for opcode, positions in raw.items():
                unreachable[opcode].append({
                    "package": package_index,
                    "offsets": sorted(positions),
                })
        except ScanError as error:
            package_errors.append({"package": package_index, "error": str(error)})

    return {
        "map": map_id,
        "sprite_size": len(sprite_data),
        "package_count": package_count,
        "scans": scans,
        "unreachable": dict(unreachable),
        "package_errors": package_errors,
    }


def serialize_report(
    lengths: list[int],
    retail: list[int],
    maps: list[dict],
    map_errors: list[dict],
    absent_map_slots: list[int],
) -> dict:
    completed = [scan for item in maps for scan in item["scans"] if scan.error is None]
    aborted = [scan for item in maps for scan in item["scans"] if scan.error is not None]

    # One instruction location is one occurrence even when several animation
    # entry points share and reach the same bytecode tail.
    reachable: dict[int, dict[tuple[int, int, int], set[str]]] = {
        opcode: defaultdict(set) for opcode in TARGETS
    }
    for scan in completed:
        for opcode, positions in scan.hits.items():
            for pc in positions:
                reachable[opcode][(scan.ident.map_id, scan.ident.package, pc)].add(scan.ident.text())

    unreachable_counts = Counter()
    unreachable_locations: dict[int, list[dict]] = defaultdict(list)
    for item in maps:
        for opcode, groups in item["unreachable"].items():
            for group in groups:
                unreachable_counts[opcode] += len(group["offsets"])
                unreachable_locations[opcode].append({"map": item["map"], **group})

    targets = {}
    for opcode in TARGETS:
        locations = []
        by_map = Counter()
        for (map_id, package, pc), entries in sorted(reachable[opcode].items()):
            locations.append({
                "map": map_id,
                "package": package,
                "offset": pc,
                "animations": sorted(entries),
            })
            by_map[map_id] += 1
        highest = []
        if by_map:
            top = max(by_map.values())
            highest = [{"map": map_id, "count": count}
                       for map_id, count in sorted(by_map.items()) if count == top]
        targets[f"0x{opcode:02X}"] = {
            "effective_length": lengths[opcode],
            "retail_table_byte": retail[opcode],
            "reachable_count": len(locations),
            "locations": locations,
            "highest_concentration": highest,
            "unreachable_raw_byte_count": unreachable_counts[opcode],
            "unreachable_locations": unreachable_locations[opcode],
        }

    reasons = Counter(scan.error for scan in aborted)
    package_errors = [
        {"map": item["map"], **error}
        for item in maps for error in item["package_errors"]
    ]
    report = {
        "format_note": (
            "func_800248D4 consumes sprite-animation packages in each field map's "
            "sprite-data section; field-script actor routine offsets belong to a different VM"
        ),
        "lengths": {
            "source": "asm/slus_006.64/data/3F290.sdata.s:D_8004FC40",
            "variable_length_opcodes": [],
            "dedicated_overrides": {
                "0xBE": 3, "0xD4": 3, "0xE2": 3, "0xFA": 4,
            },
            "effective_256": lengths,
        },
        "health": {
            "maps_decoded": len(maps),
            "absent_map_slots": absent_map_slots,
            "map_errors": map_errors,
            "packages_seen": sum(item["package_count"] for item in maps),
            "package_errors": package_errors,
            "animations_fully_decoded": len(completed),
            "animations_aborted": len(aborted),
            "abort_rate": (len(aborted) / (len(completed) + len(aborted))
                           if completed or aborted else 0.0),
            "abort_reasons": dict(reasons),
            "aborted_animations": [
                {"animation": scan.ident.text(), "start": scan.start, "error": scan.error}
                for scan in aborted
            ],
        },
        "targets": targets,
    }
    return report


def print_summary(report: dict) -> None:
    health = report["health"]
    print(
        f"maps={health['maps_decoded']} packages={health['packages_seen']} "
        f"animations_ok={health['animations_fully_decoded']} "
        f"animations_aborted={health['animations_aborted']} "
        f"abort_rate={health['abort_rate']:.2%} "
        f"absent_map_slots={len(health['absent_map_slots'])} "
        f"map_errors={len(health['map_errors'])} "
        f"package_errors={len(health['package_errors'])}"
    )
    print("opcode length reachable unreachable_raw top_maps")
    for opcode, item in report["targets"].items():
        tops = ",".join(f"{entry['map']}:{entry['count']}"
                        for entry in item["highest_concentration"]) or "-"
        print(
            f"{opcode} {item['effective_length']:>6} "
            f"{item['reachable_count']:>9} {item['unreachable_raw_byte_count']:>15} {tops}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--disc", type=Path, default=Path("disc/disc1.bin"))
    parser.add_argument(
        "--length-source", type=Path,
        default=Path("asm/slus_006.64/data/3F290.sdata.s"),
    )
    parser.add_argument("--json", type=Path, help="write the complete report as JSON")
    parser.add_argument("--maps", help="comma-separated map IDs (diagnostic subset)")
    args = parser.parse_args()

    try:
        retail = load_retail_lengths(args.length_source)
        lengths = effective_lengths(retail)
        archive = DiscArchive(args.disc)
        selected = ({int(value, 0) for value in args.maps.split(",")}
                    if args.maps else None)
        maps = []
        map_errors = []
        absent_map_slots = []
        for map_id in archive.map_ids():
            if selected is not None and map_id not in selected:
                continue
            try:
                map_file = archive.read_entry(FIELD_MAP_FIRST_ENTRY + map_id * 2)
                maps.append(scan_map(map_id, map_file, lengths))
            except AbsentArchiveEntry:
                absent_map_slots.append(map_id)
            except ScanError as error:
                map_errors.append({"map": map_id, "error": str(error)})
        archive.close()
        report = serialize_report(lengths, retail, maps, map_errors, absent_map_slots)
        print_summary(report)
        if args.json:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        return 2 if report["health"]["abort_rate"] > 0.05 else 0
    except (OSError, ScanError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
