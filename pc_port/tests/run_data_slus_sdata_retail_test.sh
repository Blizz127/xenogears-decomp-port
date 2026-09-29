#!/usr/bin/env bash
# data_slus_sdata — retail-byte certificate for pc_port/src/data_slus_sdata.c.
#
# Verifies the disc hash and main-exe section boundaries, then builds the
# generated-data test at O0/O2/UBSan and runs three negative controls that must
# each fail a named assertion:
#   zero     - drop D_8004FBB8 from the load table (stays zero)
#   truncate - load only 16 of D_8004FBB8's 32 bytes
#   sdataend - move PSX_EXE_SDATA_END / PSX_EXE_SBSS_START back to 0x800576E4
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
# Artifacts go to $TMPDIR, not pc_port/build_native/, so a concurrent
# build_port.sh cannot race with this test.
out=$(mktemp -d "${TMPDIR:-/tmp}/data_slus_sdata.XXXXXXXX")
echo "data_slus_sdata retail artifacts: $out"

python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
b = Path('disc/SLUS_006.64').read_bytes()
assert sha256(b).hexdigest() == \
    'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119', 'disc sha256'
assert b[:8] == b'PS-X EXE', 'PS-X EXE magic'
assert len(b) == 0x4A000, hex(len(b))
import struct
t_addr, t_size = struct.unpack_from('<II', b, 0x18)
assert t_addr == 0x80010000, hex(t_addr)
assert t_size == 0x49800, hex(t_size)
# Section boundaries from config/slus_006.64.yaml, corroborated by the bytes.
assert b[0x800 + (0x800592BB - 0x80010000)] != 0, 'last non-zero byte moved'
assert all(x == 0 for x in b[0x800 + (0x800592BC - 0x80010000):
                             0x800 + (0x80059800 - 0x80010000)]), 'retail .sbss non-zero'
assert all(x == 0 for x in b[0x800 + (0x800592BC - 0x80010000):
                             0x800 + (0x800592C0 - 0x80010000)]), '.data not zero'
# .sdata tail that the old 0x800576E4 loader end silently dropped.
assert any(x != 0 for x in b[0x800 + (0x800576E4 - 0x80010000):
                             0x800 + (0x800592BC - 0x80010000)]), 'empty sdata tail'
print('data_slus_sdata disc/section certificate: PASS')
PY

python3 - "$out" <<'PY'
import re
import sys
from pathlib import Path
out = Path(sys.argv[1])
src = Path('pc_port/src/data_slus_sdata.c').read_text()
# The data file declares zero storage plus a load table (retail_data.h);
# the mutants corrupt D_8004FBB8's row so the loaded bytes go wrong.
row = '    XENO_RD(D_8004FBB8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004FBB8u), 0x20),\n'
assert row in src, 'D_8004FBB8 load row not found'

# Mutant 1: D_8004FBB8 no longer loaded (stays zero).
(out / 'mutant_zero.c').write_text(src.replace(row, '', 1))

# Mutant 2: load only the first 16 bytes of D_8004FBB8.
trunc = src.replace(row, row.replace(', 0x20),', ', 0x10),'), 1)
assert trunc != src
(out / 'mutant_truncate.c').write_text(trunc)

# Mutant 3: revert the loader boundary constants.
hdr = Path('pc_port/src/psx_memory.h').read_text()
hdr2 = hdr.replace('#define PSX_EXE_SDATA_END     0x800592C0u',
                   '#define PSX_EXE_SDATA_END     0x800576E4u')
hdr2 = hdr2.replace('#define PSX_EXE_SBSS_START    0x800592C0u',
                    '#define PSX_EXE_SBSS_START    0x800576E4u')
assert hdr2 != hdr and '0x800576E4u' in hdr2
mdir = out / 'mutant_header'
mdir.mkdir()
(mdir / 'psx_memory.h').write_text(hdr2)
(mdir / 'data_slus_sdata.c').write_text(src)
print('mutants written to', out)
PY

base=(-std=gnu17 -m64 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -Ipc_port/src -Ipc_port/include_shim -Iinclude
      -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
test_src=pc_port/tests/data_slus_sdata_retail_test.c

# UBSan compiler selection: prefer gcc, fall back to clang, never skip.
echo "== production regimes =="
ubsan_cc=
if gcc "${base[@]}" -O2 -fsanitize=undefined -fno-sanitize-recover=all \
        "$test_src" -o "$out/ubsan_probe_gcc" >/dev/null 2>&1; then
    ubsan_cc=gcc
elif command -v clang >/dev/null 2>&1 && \
     clang "${base[@]}" -O2 -fsanitize=undefined -fno-sanitize-recover=all \
        "$test_src" -o "$out/ubsan_probe_clang" >/dev/null 2>&1; then
    ubsan_cc=clang
    echo "  NOTE: gcc cannot link UBSan here (missing libubsan.so.1.0.0); using clang"
else
    echo "ERROR: no compiler can build the UBSan regime; refusing to skip silently" >&2
    exit 1
fi
for opt in O0 O2 UBSan; do
    cc=gcc
    flags=(-"$opt")
    if [[ "$opt" == UBSan ]]; then
        cc="$ubsan_cc"
        flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    "$cc" "${base[@]}" "${flags[@]}" -Wall -Wextra -Werror "$test_src" -o "$out/$opt"
    "$out/$opt"
    echo "  $opt ($cc): PASS"
done

run_mutant() {
    local name="$1"; shift
    gcc "${PRE_INC[@]}" "${base[@]}" "$@" "$test_src" -o "$out/$name"
    local rc=0
    "$out/$name" >"$out/$name.log" 2>&1 || rc=$?
    if [[ "$rc" -eq 0 ]]; then
        echo "NEGATIVE CONTROL $name PASSED (should have failed)" >&2
        exit 1
    fi
    if ! grep -q 'FAIL ' "$out/$name.log"; then
        echo "NEGATIVE CONTROL $name failed without an assertion: rc=$rc" >&2
        cat "$out/$name.log" >&2
        exit 1
    fi
    echo "negative control detected: $name (rc=$rc, $(grep -m1 'FAIL ' "$out/$name.log"))"
}

echo "== mutants (each must be rejected) =="
PRE_INC=()
run_mutant zero     -O0 -DDATA_SLUS_SDATA_SRC="\"$out/mutant_zero.c\""
run_mutant truncate -O0 -DDATA_SLUS_SDATA_SRC="\"$out/mutant_truncate.c\""
# The mutant header must win the quoted-include search before -Ipc_port/src.
PRE_INC=(-I"$out/mutant_header")
run_mutant sdataend -O0 \
    -DDATA_SLUS_SDATA_SRC="\"$out/mutant_header/data_slus_sdata.c\""

echo "data_slus_sdata retail certificate PASS (3/3 negative controls rejected)"
