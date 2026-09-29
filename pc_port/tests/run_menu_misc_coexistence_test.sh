#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=pc_port/build_native/menu_misc_coexistence
mkdir -p "$OUT"

python3 - <<'PYGEN'
from pathlib import Path
import hashlib

b = Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest() == '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'

def extract(src, signature_needle):
    """Brace-aware extraction (same technique as
    run_menu_equip_core_test.sh): scan from the function's opening brace,
    tracking string/char/comment state, until the matching close brace --
    a naive '^}'-at-column-0 or next-#endif search can be fooled by nested
    braces or preprocessor blocks inside the body."""
    a = src.index(signature_needle)
    i = src.index('{', a)
    depth = 0
    in_str = in_chr = in_lc = in_bc = esc = False
    n = len(src)
    z = None
    while i < n:
        ch = src[i]
        nx = src[i + 1] if i + 1 < n else ''
        if in_lc:
            if ch == '\n':
                in_lc = False
        elif in_bc:
            if ch == '*' and nx == '/':
                in_bc = False
                i += 1
        elif in_str:
            if esc:
                esc = False
            elif ch == '\\':
                esc = True
            elif ch == '"':
                in_str = False
        elif in_chr:
            if esc:
                esc = False
            elif ch == '\\':
                esc = True
            elif ch == "'":
                in_chr = False
        else:
            if ch == '/' and nx == '/':
                in_lc = True
                i += 1
            elif ch == '/' and nx == '*':
                in_bc = True
                i += 1
            elif ch == '"':
                in_str = True
            elif ch == "'":
                in_chr = True
            elif ch == '{':
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0:
                    z = i + 1
                    break
        i += 1
    assert z is not None, signature_needle
    return src[a:z] + '\n'

src = Path('src/menu/main/misc.c').read_text()
body = ''
body += extract(src, 'extern u8 D_801C50E0[];')
body += extract(src, 'void* func_801E4A28(void* arg0) {')
body += extract(src, 'void* func_801E4D10(s32 arg0, s32 arg1) {')

for needle in ('func_801CA8C0', 'func_801E4A28', 'func_801E4D10'):
    assert needle in body, needle

Path(f'{"pc_port/build_native/menu_misc_coexistence"}/bodies.inc').write_text(body)

# D_801C50E0's 27-byte template is real retail bytes inside menu.bin
# itself. Define it here from the exact same bytes the interpreter reads
# out of the loaded ROM, instead of guessing at its extent.
def rodata_decl(name, addr, n):
    off = addr - 0x801c5000
    data = b[off:off+n]
    hexbytes = ','.join(f'0x{x:02x}' for x in data)
    return f'unsigned char {name}[{n}] = {{{hexbytes}}};\n'

rodata = rodata_decl('D_801C50E0', 0x801C50E0, 0x1B)
Path('pc_port/build_native/menu_misc_coexistence/rodata.inc').write_text(rodata)
PYGEN

BASE=(-std=gnu17 -fno-pie -include assert.h -DXENO_PC_PORT -DSKIP_ASM -DUSE_EXTENDED_PRIM_POINTERS=0 -D_LANGUAGE_C -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${BASE[@]}" "${flags[@]}" -I"$OUT" -c pc_port/tests/menu_misc_coexistence_test.c -o "$OUT/$mode.test.o"
    gcc "${BASE[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
    gcc "${BASE[@]}" "${flags[@]}" -c pc_port/src/data_game_state.c -o "$OUT/$mode.gamestate.o"
    gcc -no-pie "${flags[@]}" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" "$OUT/$mode.gamestate.o" -o "$OUT/$mode"
    timeout 60s "$OUT/$mode"
done

# One negative control: corrupt a single retail constant in the extracted
# body (func_801CA8C0's 0x4F row-quotient bias, 0x4F -> 0x50) and confirm the
# differential test actually rejects it, so a green run above means the
# harness can fail, not that it cannot.
python3 - <<'PYGEN'
from pathlib import Path
p = Path('pc_port/build_native/menu_misc_coexistence')
s = (p / 'bodies.inc').read_text()
assert s.count('q + 0x4F') == 1
mutant_dir = p / 'mutant_bias'
mutant_dir.mkdir(exist_ok=True)
(mutant_dir / 'bodies.inc').write_text(s.replace('q + 0x4F', 'q + 0x50'))
PYGEN
gcc "${BASE[@]}" -O2 -I"$OUT/mutant_bias" -I"$OUT" -c pc_port/tests/menu_misc_coexistence_test.c -o "$OUT/mutant_bias/test.o"
gcc -no-pie "$OUT/mutant_bias/test.o" "$OUT/O2.mips.o" "$OUT/O2.gamestate.o" -o "$OUT/mutant_bias/test"
if "$OUT/mutant_bias/test" >"$OUT/mutant_bias.log" 2>&1; then
    echo "FAIL: mutant survived (differential test did not catch the corrupted constant)"
    exit 1
fi
grep -qE 'Assertion|assert' "$OUT/mutant_bias.log"
echo "REJECTED mutant_bias (differential test correctly caught the corrupted constant)"
