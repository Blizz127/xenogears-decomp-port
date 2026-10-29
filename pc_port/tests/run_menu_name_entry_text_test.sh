#!/usr/bin/env bash
# Name entry (menu mode 3) text helpers: retail MIPS from the user's disc
# (disc/name_entry_menu.bin, archive dir 0x10 file 8) in the test-only
# interpreter versus the port's NON_MATCHING native bodies of
# NameEntryMenu_801CB2F0 (glyph-buffer length) and NameEntryMenu_801CB25C
# (start an edit), at O0 / O2 / UBSan; then deliberate mutants of each body
# must be rejected.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_name_entry_text
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import hashlib, re
b = Path('disc/name_entry_menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest() == 'b0c6226511fdf0d3b874cf5e8658ed1fe19fd06833c0e0d61c4762aeb6a6f1b0', 'disc/name_entry_menu.bin is not the retail file 8'
for name in ('NameEntryMenu_801CB2F0', 'NameEntryMenu_801CB25C'):
    words = re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',
                       Path(f'asm/name_entry_menu/nonmatchings/main/misc/{name}.s').read_text())
    assert words, name
    for addr, h in words:
        off = int(addr, 16) - 0x801C5000
        assert b[off:off + 4] == bytes.fromhex(h), (name, addr)
PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan; do
    flags=(-"$mode"); if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${BASE[@]}" "${flags[@]}" -c src/name_entry_menu/main/misc.c -o "$OUT/$mode.owner.o"
    gcc "${BASE[@]}" "${flags[@]}" -c pc_port/tests/menu_name_entry_text_test.c -o "$OUT/$mode.test.o"
    gcc "${BASE[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.owner.o" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
    "$OUT/$mode" | tee "$OUT/$mode.log"
    grep -q '^NAME ENTRY TEXT HELPERS PASS$' "$OUT/$mode.log"
done
python3 - <<'PYGEN'
from pathlib import Path
p = Path('pc_port/build_native/menu_name_entry_text')
s = Path('src/name_entry_menu/main/misc.c').read_text()
def body(name):
    a = s.index('NameEntryMenu_' + name + '(', s.index('#else\n/* NON_MATCHING (port body)', s.index('INCLUDE_ASM("asm/name_entry_menu/nonmatchings/main/misc", NameEntryMenu_' + name + ')')))
    a = s.rindex('\n', 0, a) + 1
    return a, s.index('\n}\n', a) + 3
mutants = {
    'len_marker': ('801CB2F0', 'glyphs[0] != 0x1F', 'glyphs[0] != 0x1E'),
    'len_hibyte': ('801CB2F0', ' || glyphs[1] != 0', ''),
    'len_bound': ('801CB2F0', 'if (i >= 0x14)', 'if (i >= 0x12)'),
    'len_unbounded': ('801CB2F0', 'if (i >= 0x14)', 'if (i >= 0x100)'),
    'edit_blank': ('801CB25C', 'glyphs[i] = 0x1C;', 'glyphs[i] = 0x1D;'),
    'edit_end': ('801CB25C', 'glyphs[0x12] = 0x1F;', 'glyphs[0x10] = 0x1F;'),
    'edit_stride': ('801CB25C', 'D_80059171 * 0x14 + i];', 'D_80059171 * 0x12 + i];'),
    'edit_copy': ('801CB25C', 'D_80059171 * 0x14 + i + 1];', 'D_80059171 * 0x14 + i];'),
}
for n, (fn, old, new) in mutants.items():
    a, z = body(fn)
    b = s[a:z]
    assert b.count(old) == 1, (n, b.count(old))
    (p / (n + '.c')).write_text(s[:a] + b.replace(old, new) + s[z:])
PYGEN
for mutant in len_marker len_hibyte len_bound len_unbounded edit_blank edit_end edit_stride edit_copy; do
    gcc "${BASE[@]}" -O2 -Isrc/name_entry_menu/main -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant"
    if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1; then echo "FAIL survived $mutant"; exit 1; fi
    grep -q '^FAIL name entry' "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
echo 'NAME ENTRY TEXT HELPERS negative controls PASS count=8'
