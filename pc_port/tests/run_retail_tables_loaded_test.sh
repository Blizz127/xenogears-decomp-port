#!/usr/bin/env bash
# Retail tables loaded from the user's disc (binary-guard task 6be4993b0e61).
#
# The port binary carries no game data; these tables used to be compiled in
# and are now zero storage filled before main() by retail_data.h:
#   PsyCross LIBGTE.C   rcossin_tbl, ratan_tbl   (psycross_gte_tables_from_disc)
#   sound.c             D_80050B78 D_80050BF0 D_80050A94 D_800509B0 D_80050824
#   animation_scripts.c D_8004FC40
#   memory.c            heap-dump strings D_80059248..D_800592B0
#   world_map_scheduler s_wm_sched_table_a / _b0 (world_map.bin Tables A, B)
# For a built xeno-port this checks that
#   1. every table is a zero-initialised object (.bss) of the retail size,
#   2. the loader filled it with exactly the retail bytes: the binary runs
#      with XENO_RETAIL_DATA_DUMP for a few seconds and each dumped object is
#      compared with the disc file window (the dumps stay in a private temp
#      directory and are never written to the tree or to out/),
#   3. PsyCross no longer defines SQRT.
# Negative control: a dump with one flipped byte must fail the comparison.
#
#   run_retail_tables_loaded_test.sh [XENO_PORT]
set -euo pipefail
cd "$(dirname "$0")/../.."
BIN=${1:-pc_port/build_native/xeno-port}
DUMP=$(mktemp -d /tmp/retail_tables_dump.XXXXXXXX)
trap 'rm -rf "$DUMP"' EXIT
nm -S --defined-only "$BIN" > "$DUMP/nm.txt"
XENO_RETAIL_DATA_DUMP="$DUMP" SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
    timeout 20 "$BIN" > "$DUMP/run.log" 2>&1 || true
python3 - "$DUMP" <<'PY'
import sys
from hashlib import sha256
from pathlib import Path
dump = Path(sys.argv[1])
SLUS, WM = 'SLUS_006.64', 'world_map.bin'
SHA = {SLUS: 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119',
       WM: '4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70'}
img = {}
for f, h in SHA.items():
    img[f] = Path('disc', f).read_bytes()
    assert sha256(img[f]).hexdigest() == h, f
def off(f, v):
    return 0x800 + v - 0x80010000 if f == SLUS else v - 0x8006FAF0
TABLES = [('rcossin_tbl', SLUS, 0x800523F0, 0x4000), ('ratan_tbl', SLUS, 0x80057030, 0x802),
          ('D_80050B78', SLUS, 0x80050B78, 120), ('D_80050BF0', SLUS, 0x80050BF0, 6144),
          ('D_80050A94', SLUS, 0x80050A94, 228), ('D_800509B0', SLUS, 0x800509B0, 228),
          ('D_80050824', SLUS, 0x80050824, 128), ('D_8004FC40', SLUS, 0x8004FC40, 256),
          ('s_wm_sched_table_a', WM, 0x80099E8C, 120), ('s_wm_sched_table_b0', WM, 0x80099F0C, 8)]
for name, size in (('D_80059248', 6), ('D_80059250', 5), ('D_80059258', 4), ('D_8005925C', 3),
                   ('D_80059260', 4), ('D_80059264', 2), ('D_80059268', 5), ('D_80059270', 8),
                   ('D_80059278', 8), ('D_80059280', 8), ('D_80059288', 6), ('D_80059290', 8),
                   ('D_80059298', 5), ('D_800592A0', 5), ('D_800592A8', 8), ('D_800592B0', 6)):
    TABLES.append((name, SLUS, int(name[2:], 16), size))
syms = {}
for line in (dump / 'nm.txt').read_text().splitlines():
    p = line.split()
    if len(p) == 4:
        syms.setdefault(p[3], []).append((p[2], int(p[1], 16)))
def check(name, f, v, size, data):
    want = img[f][off(f, v):off(f, v) + size]
    return data == want
fails = 0
for name, f, v, size in TABLES:
    kinds = syms.get(name, [])
    ok_sym = len(kinds) == 1 and kinds[0][0] in 'bB' and kinds[0][1] == size
    p = dump / (name + '.bin')
    data = p.read_bytes() if p.exists() else b''
    ok_bytes = check(name, f, v, size, data)
    print('%-20s %s+0x%05X %5d  sym=%s bytes=%s' % (name, f, off(f, v), size,
          'bss' if ok_sym else 'BAD %r' % kinds, 'retail' if ok_bytes else 'MISMATCH'))
    fails += (not ok_sym) + (not ok_bytes)
if 'SQRT' in syms:
    print('FAIL SQRT still defined', syms['SQRT']); fails += 1
# Negative control: one flipped byte must be caught.
name, f, v, size = TABLES[0]
bad = bytearray((dump / (name + '.bin')).read_bytes()); bad[size // 2] ^= 1
assert not check(name, f, v, size, bytes(bad)), 'negative control not detected'
print('negative control detected: flipped byte in', name)
print('RETAIL TABLES LOADED %s (%d tables, %d failures)' % ('PASS' if fails == 0 else 'FAIL',
                                                              len(TABLES), fails))
sys.exit(1 if fails else 0)
PY
