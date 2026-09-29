#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_a1_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE A1 artifacts: $TEST_OUT"
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
    ('a1_handler', 0x800219ac, 0x80021a44, '549035ea5e3f3f8ea8ab7f530ce2b656471180408026fca6cdd159fd764866e5')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xa1 - 0x8a) * 4 - 0x8000f800)[0] == 0x800219ac
for path in ['src/slus_006.64/system/animation_scripts.c', 'pc_port/src/battle_mips_adapter.c',
             'pc_port/tests/sprite_dispatch_a1_retail_test.c', 'pc_port/tests/run_sprite_dispatch_a1_retail_test.sh']:
    data = Path(path).read_bytes()
    pins[path] = sha256(data).hexdigest()
    (out / Path(path).name).write_bytes(data)
pins['scope'] = 'Full native and interpreted retail dispatcher; no dependency calls; valid packed pointer aliases; complete fixture and guards; no rendering or register-state claim.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 source=$2 opt=$3
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    if ! gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM \
        -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$source" \
        -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1; then
        cat "$TEST_OUT/$name.build.log" >&2
        exit 2
    fi
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections \
        pc_port/tests/sprite_dispatch_a1_retail_test.c pc_port/src/battle_mips_adapter.c \
        "$TEST_OUT/$name.body.o" -o "$TEST_OUT/$name.test"
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
if [ "$failed" != 0 ]; then
    echo "SPRITE A1 RED: see $TEST_OUT (source snapshots, retail pins and all build-mode logs)" >&2
    exit 1
fi

# These controls modify only retained scratch copies of the native source.
# Their oracle remains the same pinned retail dispatcher and input image.
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
source = (out / 'animation_scripts.c').read_text()
start = source.index('    case 0xA1: { /* 800219AC..80021A40: set vertical velocity. */')
end = source.index('    case ', start + len('    case 0xA1:'))
body = source[start:end]
assert body.count('return;') == 1 and body.count('case ') == 1

def replace_once(text, old, new):
    assert text.count(old) == 1, repr(old)
    return text.replace(old, new)

controls = []
def mutate(name, replacements, mode='O2', expected='mismatch'):
    changed = body
    for old, new in replacements:
        changed = replace_once(changed, old, new)
    assert changed != body
    result = source[:start] + changed + source[end:]
    path = out / (name + '.c')
    path.write_text(result)
    controls.append({'name': name, 'mode': mode, 'expected': expected,
                     'sha256': sha256(result.encode()).hexdigest()})

mutate('unsigned-operand', [('(u32)(s32)(s8)((u8*)operands)[0]', '(u32)((u8*)operands)[0]')])
mutate('unsigned-scale', [('(u32)(s32)(s16)AnimationRead16(p + 0x82)', '(u32)AnimationRead16(p + 0x82)')])
mutate('missing-factor-increment', [('(u32)D_80059198 + 1u', '(u32)D_80059198')])
mutate('signed-factor-increment', [('(u32)D_80059198 + 1u', '(u32)(D_80059198 + 1)')],
       mode='UBSan', expected='signed-overflow')
arithmetic = '''            value = (u32)(s32)(s8)((u8*)operands)[0] << 4;
            value *= (u32)D_80059198 + 1u;
            value *= (u32)(s32)(s16)AnimationRead16(p + 0x82);
            if ((s32)value < 0) value += 0xFFFu;
            value = (u32)((s32)value >> 12) << 8;'''
mutate('untruncated-products', [(arithmetic, '''            value = (u32)(((int64_t)(s8)((u8*)operands)[0] * 16 *
                            ((u32)D_80059198 + 1u) *
                            (s16)AnimationRead16(p + 0x82)) / 4096) << 8;''')])
mutate('missing-negative-rounding', [('            if ((s32)value < 0) value += 0xFFFu;\n', '')])
mutate('wrong-override-gate', [('AnimationRead32(p + 0xA8) & 1u', 'AnimationRead32(p + 0xA8) & 2u')])
mutate('zero-override-used', [('if (value == 0)', 'if (!(AnimationRead32(p + 0xA8) & 1u))')])
mutate('unmasked-divisor', [('(AnimationRead32(p + 0xAC) >> 7) & 0xFFFu', 'AnimationRead32(p + 0xAC) >> 7')])
mutate('unsigned-division', [('(u32)((s32)numerator / (s32)divisor)', '(u32)(numerator / divisor)')])
mutate('untruncated-dividend', [('u32 numerator;', 'int64_t numerator;'),
       ('numerator = AnimationRead32(p + 0x10) << 8;',
        'numerator = (int64_t)(s32)AnimationRead32(p + 0x10) * 256;'),
       ('(u32)((s32)numerator / (s32)divisor)', '(u32)(numerator / divisor)')])
mutate('zero-divisor-zero-result', [('((s32)numerator < 0 ? 1u : 0xFFFFFFFFu)', '0u')])
mutate('early-velocity-store', [('        u8* p = pSpriteData;\n',
       '        u8* p = pSpriteData;\n        AnimationWrite32(p + 0x10, 0);\n')],
       expected='alias-mismatch')
mutate('adjacent-field-corruption', [('        return;',
       '        AnimationWrite32(p + 0x14, 0);\n        return;')])
assert len(controls) == 14
(out / 'controls.json').write_text(json.dumps(controls, indent=2) + '\n')
(out / 'controls.tsv').write_text(''.join(c['name'] + '\t' + c['mode'] + '\t' + c['expected'] + '\n'
                                        for c in controls))
PY

while IFS=$'\t' read -r name mode expected; do
    build_case "$name" "$TEST_OUT/$name.c" "$mode"
    status=0
    "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
    python3 - "$name" "$expected" "$status" <<'PY'
from pathlib import Path
import os, re, sys
name, expected, status = sys.argv[1:]
log = (Path(os.environ['TEST_OUT']) / (name + '.log')).read_text()
assert int(status) != 0, 'Negative control survived: ' + name
if expected == 'signed-overflow':
    assert 'runtime error: signed integer overflow: 2147483647 + 1' in log, log
    detail = next(line for line in log.splitlines() if 'runtime error:' in line)
else:
    match = re.search(r'^SPRITE A1 FAIL case=(\d+) .*$', log, re.M)
    assert int(status) == 1 and match, 'Expected observable memory mismatch: ' + log
    if expected == 'alias-mismatch':
        # The mutation must preserve all 294913 external-input cases first.
        assert int(match[1]) >= 294913, log
    detail = match[0]
print('SPRITE A1 control rejected ' + name + ': ' + detail)
PY
done < "$TEST_OUT/controls.tsv"

python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path:
        assert sha256(Path(path).read_bytes()).hexdigest() == expected, 'Source changed during test: ' + path
PY
echo "SPRITE A1 GREEN: all three modes and 14 scratch-only controls; $TEST_OUT"
