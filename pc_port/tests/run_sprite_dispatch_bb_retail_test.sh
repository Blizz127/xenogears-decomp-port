#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_bb_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE BB artifacts: $TEST_OUT"
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
    ('bb_handler', 0x80020e14, 0x80020e30, '3f2e914b8b054145467eb6df9cc8992b67ecc5a51c2b0eaab302f264bf0a3623')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xbb - 0x8a) * 4 - 0x8000f800)[0] == 0x80020e14
for path in ['src/slus_006.64/system/animation_scripts.c', 'pc_port/src/battle_mips_adapter.c',
             'pc_port/tests/sprite_dispatch_bb_retail_test.c', 'pc_port/tests/run_sprite_dispatch_bb_retail_test.sh']:
    data = Path(path).read_bytes()
    pins[path] = sha256(data).hexdigest()
    (out / Path(path).name).write_bytes(data)
pins['scope'] = 'Full native and interpreted retail dispatcher; no dependency calls; all halfword/operand pairs and destination/adjacent aliases; complete fixture and guards; no rendering or register-state claim.'
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
        pc_port/tests/sprite_dispatch_bb_retail_test.c pc_port/src/battle_mips_adapter.c \
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
    echo "SPRITE BB RED: see $TEST_OUT (source snapshots, retail pins and all build-mode logs)" >&2
    exit 1
fi

# Control bodies replace only the bounded BB case in retained scratch source.
# The full production dispatcher is compared before any control is generated;
# each faulty body then faces the identical retail instruction oracle.
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
source = (out / 'animation_scripts.c').read_text()
start = source.index('    case 0xBB:')
end = source.index('    case ', start + len('    case '))
original = source[start:end]
assert original.count('return;') == 1 and original.count('case ') == 1
assert 'AnimationRead16(p + 0x30)' in original and 'AnimationWrite16(p + 0x30,' in original
controls = []

def mutate(name, statements, expected='mismatch'):
    body = '    case 0xBB: {\n        u8* p = pSpriteData;\n' + statements + '\n        return;\n    }\n'
    assert body != original
    result = source[:start] + body + source[end:]
    (out / (name + '.c')).write_text(result)
    controls.append({'name': name, 'expected': expected,
                     'sha256': sha256(result.encode()).hexdigest()})

mutate('missing-update', '        (void)p;')
mutate('unsigned-delta', '''        s32 delta = ((u8*)operands)[0];
        AnimationWrite16(p + 0x30, (u16)(AnimationRead16(p + 0x30) + delta));''')
mutate('subtract-delta', '''        s32 delta = (s8)((u8*)operands)[0];
        AnimationWrite16(p + 0x30, (u16)(AnimationRead16(p + 0x30) - delta));''')
mutate('missing-old-value', '''        s32 delta = (s8)((u8*)operands)[0];
        AnimationWrite16(p + 0x30, (u16)delta);''')
mutate('wrong-destination', '''        s32 delta = (s8)((u8*)operands)[0];
        AnimationWrite16(p + 0x32, (u16)(AnimationRead16(p + 0x30) + delta));''')
mutate('wide-store', '''        s32 delta = (s8)((u8*)operands)[0];
        AnimationWrite32(p + 0x30, (u16)(AnimationRead16(p + 0x30) + delta));''')
mutate('late-low-byte-operand', '''        u16 old = AnimationRead16(p + 0x30);
        p[0x30] = (u8)(old + (s8)((u8*)operands)[0]);
        p[0x31] = (u8)((u16)(old + (s8)((u8*)operands)[0]) >> 8);''', expected='low-alias')
mutate('late-high-byte-operand', '''        u16 old = AnimationRead16(p + 0x30);
        p[0x31] = (u8)((u16)(old + (s8)((u8*)operands)[0]) >> 8);
        p[0x30] = (u8)(old + (s8)((u8*)operands)[0]);''', expected='high-alias')
mutate('adjacent-byte-corruption', '''        s32 delta = (s8)((u8*)operands)[0];
        AnimationWrite16(p + 0x30, (u16)(AnimationRead16(p + 0x30) + delta));
        p[0x2F] = 0;''')
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
match = re.search(r'^SPRITE BB FAIL case=(\d+) .*$', log, re.M)
assert int(status) == 1 and match, 'Expected observable mismatch from ' + name + ': ' + log
if expected in ('low-alias', 'high-alias'):
    # The complete 65536x256 independent domain must pass before an alias fails.
    assert int(match[1]) >= 16777217, log
    assert ('alias=1' if expected == 'low-alias' else 'alias=2') in match[0], log
print('SPRITE BB control rejected ' + name + ': ' + match[0])
PY
done < "$TEST_OUT/controls.tsv"
verify_sources
echo "SPRITE BB GREEN: all three modes and 9 scratch-only controls; $TEST_OUT"
