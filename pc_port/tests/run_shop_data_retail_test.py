#!/usr/bin/env python3
"""Compare the loaded native data image (filled from disc/shop_menu.bin by
retail_data.h) and every alias with the disc."""
import ctypes
import hashlib
import os
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
os.chdir(root)
raw = Path('disc/shop_menu.bin').read_bytes()
assert hashlib.sha256(raw).hexdigest() == '7890e14bcabddcf85368de10783ecff8daa166dc5ac254e06db5e27959cab4cf'
expected = raw[0xcf50:0xd800]
symbols = re.findall(r'^dlabel (D_[0-9A-F]+)', Path('asm/shop_menu/data/CF50.data.s').read_text(), re.M)
source = Path('pc_port/src/data_shop_menu.c')
with tempfile.TemporaryDirectory(prefix='xeno-shop-data-') as temp:
    out = Path(temp)
    def build_check(src, name):
        library = out / (name + '.so')
        subprocess.run(['gcc', '-shared', '-fPIC', '-Ipc_port/src', str(src), '-o', str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        image = (ctypes.c_ubyte * len(expected)).in_dll(lib, 'xeno_shop_data')
        assert bytes(image) == expected, 'retail data bytes'
        base = ctypes.addressof(image)
        for symbol in symbols:
            address = ctypes.addressof(ctypes.c_ubyte.in_dll(lib, symbol))
            assert address - base == int(symbol[2:], 16) - 0x801d1f50, symbol
    build_check(source, 'native')
    for name, old, new in [('value', 'XENO_RD_MENU_OFF(0x801D1F50u)', 'XENO_RD_MENU_OFF(0x801D1F4Fu)'), ('alias', 'xeno_shop_data + 0x4\\n', 'xeno_shop_data + 0x8\\n')]:
        text = source.read_text()
        assert old in text
        mutant = out / (name + '.c')
        mutant.write_text(text.replace(old, new, 1))
        try:
            build_check(mutant, name)
        except AssertionError:
            print('SHOP DATA MUTANT REJECTED:', name)
        else:
            raise AssertionError('mutant survived: ' + name)
print(f'SHOP DATA PASS bytes={len(expected)} aliases={len(symbols)}')
