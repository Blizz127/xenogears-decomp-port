"""Detect two verified ordered callback rows in dispatch code or physical data."""
from collections import deque
import hashlib
import re
import struct

# Each digest is over the full sequence of little-endian 32-bit guest addresses.
SOUND = (9, 'f32138913e12e6c31982910dd9ee19550b23a766e9e8978da417a3a9d50a117a',
         'original sound dispatch address sequence')
WORLD = (16, '59770f536a418059e50a7e1fc665eb6173bb1ece75977d3ad88a85730e2c739a',
         'original world-map dispatch address sequence')

# Comments do not separate statements. Quoted text may contain code examples.
NON_CODE = re.compile(rb'//[^\r\n]*|/\*.*?(?:\*/|\Z)|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
NUMBER = rb'(0[xX][a-fA-F0-9]{1,8}|0[0-7]{1,11}|[0-9]{1,10})[uUlL]*'
CASE = re.compile(rb'\bcase\s+' + NUMBER + rb'\s*:\s*return\s+[^;{}]+;')
BRANCH = re.compile(rb'\bif\s*\(\s*([A-Za-z_]\w*)\s*==\s*' + NUMBER +
                    rb'\s*\)\s*(?:\{\s*)?return\s+[^;{}]+;(?:\s*\})?')
# A packed row always contains one 0x80 per little-endian address. Filter
# before hashing full windows. Consuming just one byte permits overlapping rows.
PACKED = re.compile(rb'\x80(?=(?:.{3}\x80){8})', re.S)


def source_row(code, pattern, rule):
    size, digest, _ = rule
    values = deque(maxlen=size)
    end = 0
    previous_key = None
    for statement in pattern.finditer(code):
        key = statement[1] if pattern is BRANCH else None
        if key != previous_key or code[end:statement.start()].strip():
            values.clear()
        previous_key = key
        end = statement.end()
        token = statement[2] if pattern is BRANCH else statement[1]
        radix = 16 if token[:2].lower() == b'0x' else 8 if token.startswith(b'0') else 10
        try:
            value = int(token, radix)
        except ValueError:
            values.clear()
            continue
        if value >> 24 != 0x80:
            values.clear()
            continue
        values.append(value)
        if len(values) == size:
            row = struct.pack('<' + 'I' * size, *values)
            if hashlib.sha256(row).hexdigest() == digest:
                return True
    return False


def check_address_dispatch(name, data):
    hits = set()
    if b'\x80' in data:
        for match in PACKED.finditer(data):
            start = match.start() - 3
            if start >= 0:
                for count, digest, label in (SOUND, WORLD):
                    end = start + count * 4
                    if end <= len(data) and hashlib.sha256(data[start:end]).hexdigest() == digest:
                        hits.add(label)
    if b'\0' not in data and b'return' in data and (b'case' in data or b'==' in data):
        code = NON_CODE.sub(lambda m: b' ' if m[0].startswith(b'/') else b'""', data)
        for pattern, rule in ((CASE, SOUND), (BRANCH, WORLD)):
            if source_row(code, pattern, rule):
                hits.add(rule[2])
    return sorted(hits)
