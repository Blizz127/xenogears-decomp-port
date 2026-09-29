#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_b4_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE B4 artifacts: $TEST_OUT"
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
    ('b4_handler', 0x80021480, 0x80021494, '7b06aa48703e3378598627a485e1f786756eea203eac72199efa66a22d4ee943'),
    ('stack_push_u8', 0x80021ca0, 0x80021cc4, 'ec0747f40b2ddd31c8017757871fa791d8ce10f75e40dad63f0345955c3373b0')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xb4 - 0x8a) * 4 - 0x8000f800)[0] == 0x80021480
for path in ['src/slus_006.64/system/animation_scripts.c', 'pc_port/src/battle_mips_adapter.c',
             'pc_port/tests/sprite_dispatch_b4_retail_test.c', 'pc_port/tests/run_sprite_dispatch_b4_retail_test.sh']:
    data = Path(path).read_bytes()
    pins[path] = sha256(data).hexdigest()
    (out / Path(path).name).write_bytes(data)
pins['scope'] = 'Full native and interpreted retail dispatcher; real stack helpers; no unrelated dependency calls; signed stack range and cursor/destination aliases; complete fixture and guards; no rendering or register-state claim.'
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
        pc_port/tests/sprite_dispatch_b4_retail_test.c pc_port/src/battle_mips_adapter.c \
        "$TEST_OUT/$name.body.o" -o "$TEST_OUT/$name.test"
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
    echo "SPRITE B4 RED: see $TEST_OUT (source snapshots, retail pins and all build-mode logs)" >&2
    exit 1
fi

# Controls change only the B4 handler or its actual helper in scratch copies.
# They all use the unchanged retail instruction oracle and physical inputs.
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
source = (out / 'animation_scripts.c').read_text()
handler_start = source.index('    case 0xB4: /* 80021480..90: capture the byte before the stack push. */')
handler_end = source.index('    case 0xB8:', handler_start)
helper_start = source.index('\nvoid AnimScriptStackPushU8(SpriteData* pSpriteData, u8 value) {')
helper_end = source.index('\nvoid AnimScriptStackPushU16(', helper_start)
handler = source[handler_start:handler_end]
helper = source[helper_start:helper_end]
assert handler.count('return;') == 1 and handler.count('case ') == 1
assert helper.count('void ') == 1 and helper.count('}') == 1
controls = []

def mutate(name, seam, replacements, expected='mismatch'):
    start, end, body = ((handler_start, handler_end, handler) if seam == 'handler'
                        else (helper_start, helper_end, helper))
    changed = body
    for old, new in replacements:
        assert changed.count(old) == 1, repr(old)
        changed = changed.replace(old, new)
    assert changed != body
    result = source[:start] + changed + source[end:]
    (out / (name + '.c')).write_text(result)
    controls.append({'name': name, 'seam': seam, 'expected': expected,
                     'sha256': sha256(result.encode()).hexdigest()})

call = '        AnimScriptStackPushU8((SpriteData*)pSpriteData, ((u8*)operands)[0]);'
mutate('missing-push', 'handler', [(call, '        /* missing push */')])
mutate('zero-pushed-value', 'handler', [('((u8*)operands)[0]', '0')])
mutate('unsigned-stack-cursor', 'helper', [('s8 idx = (s8)--pData[0x8C];',
                                         'u8 idx = (u8)--pData[0x8C];')])
mutate('missing-cursor-decrement', 'helper', [('(s8)--pData[0x8C]', '(s8)pData[0x8C]')])
mutate('post-decrement-index', 'helper', [('(s8)--pData[0x8C]', '(s8)pData[0x8C]--')])
mutate('wrong-stack-base', 'helper', [('pData[0x8E + idx]', 'pData[0x8F + idx]')])
mutate('cursor-store-after-data', 'helper', [
    ('s8 idx = (s8)--pData[0x8C];', 's8 idx = (s8)(u8)(pData[0x8C] - 1);'),
    ('    pData[0x8E + idx] = value;',
     '    pData[0x8E + idx] = value;\n    pData[0x8C] = (u8)idx;')], expected='cursor-overlap')
mutate('late-operand-capture', 'handler', [(call, '''        {
            u8* p = pSpriteData;
            s8 idx = (s8)--p[0x8C];
            p[0x8E + idx] = ((u8*)operands)[0];
        }''')], expected='operand-alias')
mutate('adjacent-byte-corruption', 'helper', [('    pData[0x8E + idx] = value;',
       '    pData[0x8E + idx] = value;\n    pData[0x8D] = 0;')])
assert len(controls) == 9
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
match = re.search(r'^SPRITE B4 FAIL case=(\d+) .*$', log, re.M)
assert int(status) == 1 and match, 'Expected observable mismatch from ' + name + ': ' + log
if expected == 'cursor-overlap':
    assert 'cursor=ff' in match[0] and 'destination=08c' in match[0], log
elif expected == 'operand-alias':
    # All independent cursor/operand cases must pass before a cursor alias fails.
    assert int(match[1]) >= 262145 and 'alias=1' in match[0], log
print('SPRITE B4 control rejected ' + name + ': ' + match[0])
PY
done < "$TEST_OUT/controls.tsv"
verify_sources
echo "SPRITE B4 GREEN: all three modes and 9 scratch-only controls; $TEST_OUT"
