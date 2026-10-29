#!/usr/bin/env python3
"""Compare RESULTS snapshots after TWO stable reference runs.

Only establishes parity for recorded memory ranges and boundaries. Does not
establish victory, timing/audio/visual/save identity, native execution or full
RAM parity. Private binary snapshots may contain retail code: never publish.
"""
import argparse
import hashlib
from pathlib import Path
import re

RECORD = re.compile(r'S (\d+) (\d+) (entry|render|return) (\d+) ra=([0-9a-f]+) sp=([0-9a-f]+) status=OK')
SNAPSHOT_SIZE = 0x235C + 0x6500 + 11 * 0x170 + 32 * 72 + 8 * 64 + 0x40


def load(directory):
    text = (directory / 'snapshots.log').read_text()
    if 'FAIL' in text or not text.startswith('FORMAT 1 '):
        raise ValueError(f'{directory}: failed or unrecognized trace')
    records = []
    for line in text.splitlines()[1:]:
        m = RECORD.fullmatch(line)
        if not m:
            raise ValueError(f'{directory}: malformed snapshot record')
        battle, invocation, phase, render, ra, sp = m.groups()
        snapshot = directory / f'b{battle}-r{invocation}-{phase}-{render}.bin'
        data = snapshot.read_bytes()
        if len(data) != SNAPSHOT_SIZE:
            raise ValueError(f'{snapshot}: truncated or unexpected length')
        records.append((m.groups(), hashlib.sha256(data).hexdigest()))
    if not records or records[0][0][2] != 'entry' or records[-1][0][2] != 'return':
        raise ValueError(f'{directory}: no complete RESULTS entry/return route')
    # Every recorded invocation needs exactly one opening and closing boundary.
    active = None
    render_index = 0
    for record, _ in records:
        battle, invocation, phase, render, ra, sp = record
        key = (battle, invocation, ra, sp)
        if phase == 'entry':
            if active is not None or render != '0':
                raise ValueError(f'{directory}: nested or malformed entry')
            active, render_index = key, 0
        else:
            if active != key or int(render) != render_index:
                raise ValueError(f'{directory}: inconsistent boundary')
            if phase == 'render':
                render_index += 1
            else:
                active = None
    if active is not None:
        raise ValueError(f'{directory}: unreturned invocation')
    return records


def compare(reference_a, reference_b, native):
    a, b, c = map(load, (reference_a, reference_b, native))
    if a != b:
        raise ValueError('reference runs differ: native parity cannot be assessed')
    if a != c:
        raise ValueError('native snapshots/boundaries differ from stable reference')
    return len(a)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('reference_a', 'reference_b', 'native'):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    try:
        count = compare(args.reference_a, args.reference_b, args.native)
    except (ValueError, OSError) as error:
        print('RESULTS ROUTE SNAPSHOT FAIL:', error)
        return 1
    print(f'RESULTS ROUTE SNAPSHOT PASS boundaries={count}; bounded memory parity only')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
