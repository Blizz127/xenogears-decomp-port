#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_c1_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE C1 artifacts: $TEST_OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os, struct
out = Path(os.environ['TEST_OUT'])
image = Path('disc/SLUS_006.64').read_bytes()
assert sha256(image).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
pins = {'SLUS_006.64': sha256(image).hexdigest()}
for name, start, end, expected in [
    ('dispatch_table', 0x800183d8, 0x800185a4, 'ee73be97683cbf2affa5db0d128e5083ac4b273da3bc5c2f0bfc12bfb75ee0f8'),
    ('full_dispatcher', 0x8001fbe4, 0x80021ad8, '7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c'),
    ('c1_handler', 0x800201f0, 0x800202f4, '041b7c442561e5a58678a1e8792ed70e5ca52d1b053206f00246672d05a82123'),
    ('rand', 0x8003fa38, 0x8003fa68, '8efec2e9f765b1fe90da7f814c3864d0a4c87d4df7e4bc67af491d5a30c21e81')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xc1 - 0x8a) * 4 - 0x8000f800)[0] == 0x800201f0
pins['retail_rot_matrix'] = {'start': '0x8003f738', 'end': '0x8003f8b0',
                            'sha256': sha256(image[0x2ff38:0x300b0]).hexdigest()}
for path in ['src/slus_006.64/system/animation_scripts.c', 'pc_port/src/battle_mips_adapter.c', 'pc_port/src/psyq_compat.c', 'pc_port/src/game_overrides.c', 'src/slus_006.64/system/temp1.c',
             'pc_port/include_shim/psyq/libgte.h', 'pc_port/src/port_compat.h',
             'pc_port/extern/PsyCross/src/psx/LIBGTE.C', 'pc_port/extern/PsyCross/src/psx/INLINE_C.C',
             'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp', 'pc_port/extern/PsyCross/src/gte/half_float.cpp',
             'pc_port/tests/sprite_dispatch_c1_retail_test.c', 'pc_port/tests/run_sprite_dispatch_c1_retail_test.sh']:
    data = Path(path).read_bytes()
    pins[path] = sha256(data).hexdigest()
    (out / Path(path).name).write_bytes(data)
source = Path('src/slus_006.64/system/temp1.c').read_text()
start = source.index('s32 func_80022CAC(void* pSpriteData, s32 value)\n{')
end = source.index('\n}\n', start) + 3
helper = source[start:end]
assert helper.count('s32 func_80022CAC(') == 1 and helper.count('return ') == 1
(out / 'slow_helper.c').write_text('#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef int32_t s32; typedef int64_t s64;\n' + helper)
pins['slow_helper_sha256'] = sha256(helper.encode()).hexdigest()
pins['scope'] = 'Full native and interpreted retail dispatcher; whole native dispatcher/RNG/GTE TUs; verbatim native slow helper; full retail instructions with shared PsyCross GTE backend; whole fixture/seed, three RNG calls and nine observable rotation halfwords, varied retail stack padding; no hardware-GTE, padding, full register-state or rendering claim.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 source=$2 opt=$3 compat=${4:-pc_port/src/psyq_compat.c}
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    if ! gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM \
        -Ipc_port/include_shim -Iinclude -c "$source" \
        -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1; then
        cat "$TEST_OUT/$name.build.log" >&2
        exit 2
    fi
    if ! gcc -std=gnu17 -fpermissive "${common[@]}" -include assert.h -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
        -Ipc_port/include_shim -Iinclude -Ipc_port/src -c "$compat" \
        -o "$TEST_OUT/$name.compat.o" >> "$TEST_OUT/$name.build.log" 2>&1; then
        cat "$TEST_OUT/$name.build.log" >&2
        exit 2
    fi
    gcc -std=gnu17 "${common[@]}" -c "$TEST_OUT/slow_helper.c" -o "$TEST_OUT/$name.slow.o"
    for component in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp gte/half_float.cpp; do
        component_name=${component##*/}
        g++ -std=c++17 "${common[@]}" -fpermissive -w -include pc_port/src/port_compat.h \
            -c "pc_port/extern/PsyCross/src/$component" -o "$TEST_OUT/$name.$component_name.o"
    done
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections \
        pc_port/tests/sprite_dispatch_c1_retail_test.c pc_port/src/battle_mips_adapter.c \
        "$TEST_OUT/$name.body.o" "$TEST_OUT/$name.compat.o" "$TEST_OUT/$name.slow.o" \
        "$TEST_OUT/$name.LIBGTE.C.o" "$TEST_OUT/$name.INLINE_C.C.o" "$TEST_OUT/$name.PsyX_GTE.cpp.o" "$TEST_OUT/$name.half_float.cpp.o" -lstdc++ -lm \
        -Wl,--wrap=rand,--wrap=ReadGeomOffset,--wrap=ScaleMatrixL,--wrap=ScaleMatrix,--wrap=ApplyMatrixSV,--wrap=MulMatrix0 -o "$TEST_OUT/$name.test"
}

verify_sources() {
    python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path:
        assert sha256(Path(path).read_bytes()).hexdigest() == expected, 'Source changed during test: ' + path
PY
}

failed=0
for opt in O0 O2 UBSan; do
    build_case "$opt" src/slus_006.64/system/animation_scripts.c "$opt"
    if "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1; then
        cat "$TEST_OUT/$opt.log"
    else
        cat "$TEST_OUT/$opt.log" >&2
        failed=1
    fi
done
verify_sources
if [ "$failed" != 0 ]; then
    echo "SPRITE C1 RED: see $TEST_OUT (source snapshots, retail pins and all build-mode logs)" >&2
    exit 1
fi

# Each control changes only C1 or its dedicated matrix helper in a retained
# native source copy. The real RNG, retail oracle and shared GTE stay intact.
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
source = (out / 'animation_scripts.c').read_text()
start = source.index('    case 0xC1:')
end = source.index('    case ', start + len('    case '))
body = source[start:end]
assert body.count('case ') == 1 and body.count('return;') == 1
controls = []
def mutate(name, replacements, expected='mismatch'):
    changed = body
    for old, new in replacements:
        assert changed.count(old) == 1, repr(old)
        changed = changed.replace(old, new)
    assert changed != body
    result = source[:start] + changed + source[end:]
    (out / (name + '.c')).write_text(result)
    controls.append({'name': name, 'expected': expected,
                     'sha256': sha256(result.encode()).hexdigest()})
mutate('signed-operand', [('((u8*)operands)[0]', '(s32)((s8*)operands)[0]')])
mutate('unsigned-scale', [('(s16)AnimationRead16(p + 0x2C)', '(u16)AnimationRead16(p + 0x2C)')])
mutate('missing-negative-rounding', [('        if (product < 0) product += 0xFFF;\n', '')])
mutate('missing-slow-scale', [('func_80022CAC(p, product >> 12)', '(product >> 12)')])
mutate('swapped-random-angles', [('''        angles.vx = (s16)rand();
        angles.vy = (s16)rand();''', '''        angles.vy = (s16)rand();
        angles.vx = (s16)rand();''')])
mutate('missing-third-random-call', [('angles.vy = (s16)rand();', 'angles.vy = 0;')])
mutate('unsigned-translation', [('translation.vy = (s16)AnimationRead16(p + 6);',
                               'translation.vy = (u16)AnimationRead16(p + 6);')])
mutate('missing-translation-state', [('        SetTransMatrix(&matrix);\n', '')])
mutate('wrong-final-shift', [('(u32)(s32)local.vz << 16', '(u32)(s32)local.vz << 12')])
mutate('adjacent-field-corruption', [('        return;',
                                   '        AnimationWrite32(p + 0xC, 0);\n        return;')])
mutate('early-operand-read', [
    ('        radius = (s32)((u32)rand() & 0xFFu);',
     '        u32 range = ((u8*)operands)[0];\n        radius = (s32)((u32)rand() & 0xFFu);'),
    ('radius = (radius * ((u8*)operands)[0]) >> 8;', 'radius = (radius * range) >> 8;')],
    expected='seed-operand')
mutate('original-host-rot-matrix', [('AnimationC1RotMatrix(&angles, &matrix);',
                                    'RotMatrix(&angles, &matrix);')])
old = 'matrix->m[1][2] = (s16)((-cy * sx) >> 12);'
new = 'matrix->m[1][2] = (s16)-((cy * sx) >> 12);'
assert source.count(old) == 1 and source.index(old) < start
result = source.replace(old, new)
(out / 'negative-shift-order.c').write_text(result)
controls.append({'name': 'negative-shift-order', 'expected': 'mismatch',
                 'sha256': sha256(result.encode()).hexdigest()})
assert len(controls) == 13
(out / 'controls.json').write_text(json.dumps(controls, indent=2) + '\n')
(out / 'controls.tsv').write_text(''.join(c['name'] + '\t' + c['expected'] + '\n' for c in controls))
PY

while IFS=$'\t' read -r name expected; do
    build_case "$name" "$TEST_OUT/$name.c" O2
    status=0
    "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
    python3 - "$name" "$expected" "$status" <<'PY'
from pathlib import Path
import os, re, sys
name, expected, status = sys.argv[1:]
log = (Path(os.environ['TEST_OUT']) / (name + '.log')).read_text()
match = re.search(r'^SPRITE C1 FAIL case=(\d+) .*$', log, re.M)
assert int(status) == 1 and match, 'Expected observable mismatch from ' + name + ': ' + log
if expected == 'seed-operand':
    assert int(match[1]) >= 1015809, log
    assert re.search(r'alias=(17|18|19|20) ', match[0]), log
print('SPRITE C1 control rejected ' + name + ': ' + match[0])
PY
done < "$TEST_OUT/controls.tsv"
verify_sources
echo "SPRITE C1 GREEN: all three modes and 13 scratch-only controls; $TEST_OUT"
