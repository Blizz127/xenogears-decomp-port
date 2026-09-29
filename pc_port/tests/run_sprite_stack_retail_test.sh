#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_stack_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
b = Path('disc/SLUS_006.64').read_bytes()
assert sha256(b).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
for lo, hi, digest in [
    (0x80021c20, 0x80021d3c, '35be16ffc842a31b0180ee9f44c4c26c70c561d8a083cbca3b5f72baa3374625'),
    (0x8001fba4, 0x8001fbe4, '0dd76ad93a5ccc0795578b35f4ac3d70c48f39765823740874f7cc66c55c04c1'),
]:
    assert sha256(b[lo-0x8000f800:hi-0x8000f800]).hexdigest() == digest
print('SPRITE STACK retail SLUS, six helper bodies and GetArg SHA pins PASS')
PY
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    # Preserve the legacy TU's permissive flags and capture its diagnostics;
    # compile the new fixture and CPU with Werror below.
    if ! gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM \
        -Iinclude -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c src/slus_006.64/system/animation_scripts.c \
        -o "$OUT/$opt.body.o" 2> "$OUT/$opt.compile.log"; then
        cat "$OUT/$opt.compile.log" >&2
        exit 1
    fi
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" \
        -Wl,--gc-sections pc_port/tests/sprite_stack_retail_test.c \
        pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
python3 - "$OUT" <<'PY'
from pathlib import Path
import re
import sys

out = Path(sys.argv[1])
source = Path('src/slus_006.64/system/animation_scripts.c').read_text()
def extent(text, function):
    match = re.search(r'\n(?:u_char|s32|void\*|void) ' + re.escape(function) + r'\(', text)
    if not match:
        raise SystemExit(f'SPRITE STACK mutation function drift: {function}')
    start = match.start() + 1
    return start, text.index('\n}', start) + 2

names = []
layout = source
for function in ['func_8001FBA4'] + [f'AnimScriptStack{op}U{width}'
                                     for op in ['Pop', 'Push'] for width in [8, 16, 24]]:
    start, end = extent(layout, function)
    body = layout[start:end]
    assert '0x8C' in body
    for old, new in [('0x8C', '0xC8'), ('0x8E', '0xCA'), ('0x8F', '0xCB'),
                     ('0x90', '0xCC'), ('0x88', '0xC0')]:
        body = body.replace(old, new)
    layout = layout[:start] + body + layout[end:]
(out / 'layout.c').write_text(layout)
names.append('layout')

# Each control changes one defined behavior. Reload controls separately remove
# the first and second reread after writes that can alias the index byte.
mutants = {
    'signed_index': ('AnimScriptStackPopU24', 's8 idx = (s8)pData[0x8C];',
                     's32 idx = (u8)pData[0x8C];'),
    'index_update': ('AnimScriptStackPopU8', 'pData[0x8C]++;', 'pData[0x8C] += 2;'),
    'index_width': ('AnimScriptStackPopU24', 'pData[0x8C] += 3;',
                    'AnimationWrite16(pData + 0x8C, (u16)(pData[0x8C] + 3));'),
    'pop_read_order': ('AnimScriptStackPopU8',
                       'nStackValue = pData[0x8E + idx];\n    pData[0x8C]++;',
                       'pData[0x8C]++;\n    nStackValue = pData[0x8E + idx];'),
    'pop_byte_order': ('AnimScriptStackPopU24', 'b0 + b1 * 256 + b2 * 65536',
                       'b2 + b1 * 256 + b0 * 65536'),
    'pop16_return_width': ('AnimScriptStackPopU16', 's32 AnimScriptStackPopU16',
                           's16 AnimScriptStackPopU16'),
    'pop16_sign': ('AnimScriptStackPopU16', 'return (s16)(lo + hi * 256);',
                   'return (u16)(lo + hi * 256);'),
    'pop24_high_sign': ('AnimScriptStackPopU24', 'b2 * 65536', '(s8)b2 * 65536'),
    'push_byte_order': ('AnimScriptStackPushU24',
                        'pData[0x8F + idx] = (u8)(value >> 8);',
                        'pData[0x8F + idx] = (u8)(value >> 16);'),
    'push_wrap_sign': ('AnimScriptStackPushU8', 's8 idx = (s8)--pData[0x8C];',
                       's32 idx = --pData[0x8C];'),
    'push_index_update': ('AnimScriptStackPushU24', 'pData[0x8C] -= 3',
                          'pData[0x8C] -= 2'),
    'push16_no_reload': ('AnimScriptStackPushU16', 'idx = (s8)pData[0x8C];', '(void)idx;'),
    'push24_no_first_reload': ('AnimScriptStackPushU24', 'idx = (s8)pData[0x8C];', '(void)idx;', 2, 0),
    'push24_no_second_reload': ('AnimScriptStackPushU24', 'idx = (s8)pData[0x8C];', '(void)idx;', 2, 1),
    'argument_mode': ('func_8001FBA4', 'if (!(index & 0x80))', 'if (!(index & 0x40))'),
    'argument_mask': ('func_8001FBA4', '(index & 0x7F)', '(index & 0x3F)'),
    'argument_stride': ('func_8001FBA4', '(index & 0x7F)', '((index & 0x7F) * 4)'),
    'argument_index_sign': ('func_8001FBA4', '(s8)pData[0x8C]', '(u8)pData[0x8C]'),
    'argument_pointer_width': ('func_8001FBA4',
                               'return (void*)(uintptr_t)(AnimationRead32(pData + 0x88) + (index & 0x7F));',
                               'return (void*)((uintptr_t)*(void**)(pData + 0x88) + (index & 0x7F));'),
}
for name, change in mutants.items():
    function, old, new = change[:3]
    count, occurrence = change[3:] if len(change) == 5 else (1, 0)
    start, end = extent(source, function)
    body = source[start:end]
    if body.count(old) != count:
        raise SystemExit(f'SPRITE STACK mutation anchor drift: {name}')
    offset = -len(old)
    for _ in range(occurrence + 1):
        offset = body.index(old, offset + len(old))
    body = body[:offset] + new + body[offset + len(old):]
    (out / f'{name}.c').write_text(source[:start] + body + source[end:])
    names.append(name)
(out / 'mutants.list').write_text(''.join(f'{name}\n' for name in names))
PY
controls=0
while IFS= read -r mutant; do
    if ! gcc -std=gnu17 -fpermissive -O2 -include stdint.h -D_LANGUAGE_C -fno-pie \
        -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM \
        -Iinclude -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$OUT/$mutant.c" -o "$OUT/$mutant.body.o" \
        2> "$OUT/$mutant.compile.log"; then
        cat "$OUT/$mutant.compile.log" >&2
        exit 1
    fi
    clang -std=c17 -O2 -Wall -Wextra -Werror -fno-pie -no-pie -Ipc_port/src \
        -Wl,--gc-sections pc_port/tests/sprite_stack_retail_test.c \
        pc_port/src/battle_mips_adapter.c "$OUT/$mutant.body.o" -o "$OUT/$mutant.test"
    status=0
    "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" -ne 1 ] || ! rg -q '^SPRITE STACK FAIL' "$OUT/$mutant.log"; then
        echo "SPRITE STACK negative control failed: $mutant status=$status" >&2
        exit 1
    fi
    controls=$((controls + 1))
done < "$OUT/mutants.list"
echo "SPRITE STACK negative controls PASS: $controls layout/index/order/reload/return/argument regressions rejected"
