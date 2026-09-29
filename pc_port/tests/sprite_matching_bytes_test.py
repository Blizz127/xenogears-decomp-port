"""Compare individual sprite C functions after resolving their call relocations."""
from pathlib import Path
import hashlib
import json
import struct

OBJECT = Path('build/src/slus_006.64/system/temp1.c.o')
RETAIL = Path('disc/SLUS_006.64')
TARGETS = {'func_80022B2C': (0x80022B2C, 384),
           'func_80022CAC': (0x80022CAC, 48),
           'func_80022CDC': (0x80022CDC, 104)}
b = OBJECT.read_bytes()
raw = RETAIL.read_bytes()
assert hashlib.sha256(raw).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
assert b[:6] == b'\x7fELF\x01\x01'
shoff = struct.unpack_from('<I', b, 32)[0]
shsize, count, _ = struct.unpack_from('<HHH', b, 46)
sections = [struct.unpack_from('<10I', b, shoff + i * shsize) for i in range(count)]
def data(section):
    return b[section[4]:section[4] + section[5]]
symtab = next(s for s in sections if s[1] == 2)
strings = data(sections[symtab[6]])
symbols = [struct.unpack_from('<IIIBBH', data(symtab), i)
           for i in range(0, symtab[5], symtab[9])]
def name(symbol):
    return strings[symbol[0]:].split(b'\0')[0].decode()

def compare(symbol, relocation_override=None, mutate=False):
    address, size = TARGETS[name(symbol)]
    code = bytearray(data(sections[symbol[5]])[symbol[1]:symbol[1] + symbol[2]])
    relocations = []
    if len(code) != size:
        return {'bytes': len(code), 'retail_bytes': size, 'exact': False, 'reason': 'size differs'}
    for section in sections:
        if section[1] != 9 or section[7] != symbol[5]:
            continue
        for off in range(0, section[5], section[9]):
            loc, info = struct.unpack_from('<II', data(section), off)
            if not symbol[1] <= loc < symbol[1] + symbol[2]:
                continue
            target = name(symbols[info >> 8])
            assert info & 255 == 4, 'unexpected relocation type'
            assert target in TARGETS, target
            delta = loc - symbol[1]
            word = struct.unpack_from('<I', code, delta)[0]
            assert word & 0x03ffffff == 0, 'unexpected call addend'
            resolved = TARGETS[relocation_override or target][0]
            struct.pack_into('<I', code, delta, (word & 0xfc000000) | ((resolved >> 2) & 0x03ffffff))
            relocations.append({'offset': delta, 'target': target, 'address': resolved})
    if mutate:
        code[0] ^= 1
    expected = raw[address - 0x8000f800:address - 0x8000f800 + size]
    return {'bytes': len(code), 'retail_bytes': size, 'exact': code == expected,
            'relocations': relocations, 'normalized_sha256': hashlib.sha256(code).hexdigest(),
            'retail_sha256': hashlib.sha256(expected).hexdigest()}
selected = {name(s): s for s in symbols if name(s) in TARGETS}
assert set(selected) == set(TARGETS)
report = {n: compare(s) for n, s in selected.items()}
assert report['func_80022CAC']['exact']
assert report['func_80022CDC']['exact']
assert len(report['func_80022CAC']['relocations']) == 0
assert len(report['func_80022CDC']['relocations']) == 3
assert not compare(selected['func_80022CAC'], mutate=True)['exact']
assert not compare(selected['func_80022CDC'], relocation_override='func_80022CDC')['exact']
print(json.dumps({'object_sha256': hashlib.sha256(b).hexdigest(), 'functions': report,
                  'controls_rejected': ['changed instruction byte', 'wrong call relocation']}, indent=2))
