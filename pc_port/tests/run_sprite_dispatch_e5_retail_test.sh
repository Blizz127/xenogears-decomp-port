#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_e5_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE E5 artifacts: $TEST_OUT"
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
    ('e5_handler', 0x80020c20, 0x80020c50, '90966125706731396a4b9906e07a0211d50c3d18748ce5efbc0fef5ec394ec5a'),
    ('pointer_helper', 0x8001fba4, 0x8001fbe4, '0dd76ad93a5ccc0795578b35f4ac3d70c48f39765823740874f7cc66c55c04c1'),
    ('rand', 0x8003fa38, 0x8003fa68, '8efec2e9f765b1fe90da7f814c3864d0a4c87d4df7e4bc67af491d5a30c21e81')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xe5 - 0x8a) * 4 - 0x8000f800)[0] == 0x80020c20
for path in ['src/slus_006.64/system/animation_scripts.c', 'pc_port/src/battle_mips_adapter.c', 'pc_port/src/psyq_compat.c',
             'pc_port/tests/sprite_dispatch_e5_retail_test.c', 'pc_port/tests/run_sprite_dispatch_e5_retail_test.sh']:
    data = Path(path).read_bytes()
    pins[path] = sha256(data).hexdigest()
    (out / Path(path).name).write_bytes(data)
pins['scope'] = 'Full native and interpreted retail dispatcher; real pointer helper and RNG; transparent rand call counter and fail-fast guards for unrelated dependencies; complete fixture, guards and RNG seed; no rendering or register-state claim.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 source=$2 opt=$3 compat=${4:-pc_port/src/psyq_compat.c}
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    if ! gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM \
        -Iinclude -Ipc_port/include_shim -c "$source" \
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
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections \
        pc_port/tests/sprite_dispatch_e5_retail_test.c pc_port/src/battle_mips_adapter.c \
        "$TEST_OUT/$name.body.o" "$TEST_OUT/$name.compat.o" \
        -Wl,--wrap=rand,--wrap=ReadGeomOffset,--wrap=ScaleMatrixL -o "$TEST_OUT/$name.test"
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
    echo "SPRITE E5 RED: see $TEST_OUT (source snapshots, retail pins and all build-mode logs)" >&2
    exit 1
fi

# All mutations are scratch-only. The unmodified full production translation
# units pass first; each control then faces the same actual retail instructions.
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
source = (out / 'animation_scripts.c').read_text()
compat = (out / 'psyq_compat.c').read_text()
start = source.index('    case 0xE5:')
end = source.index('    case ', start + len('    case '))
original = source[start:end]
assert original.count('return;') == 1 and original.count('case ') == 1
assert original.count('func_8001FBA4(') == 1 and original.count('rand()') == 1
controls = []

def save(name, animation, random_source, expected='mismatch'):
    assert animation != source or random_source != compat
    (out / (name + '.c')).write_text(animation)
    (out / (name + '.compat.c')).write_text(random_source)
    controls.append({'name': name, 'expected': expected,
        'animation_sha256': sha256(animation.encode()).hexdigest(),
        'compat_sha256': sha256(random_source.encode()).hexdigest()})

def mutate(name, statements, expected='mismatch'):
    body = '    case 0xE5: {\n' + statements + '\n        return;\n    }\n'
    save(name, source[:start] + body + source[end:], compat, expected)

resolve = '        u8* destination = func_8001FBA4((SpriteData*)pSpriteData, (u8*)operands);\n'
random = '        u32 random = (u32)rand() & 0xFFu;\n'
write = '        *destination = (u8)((random * ((u8*)operands)[1]) >> 8);'
mutate('missing-random-call', resolve + '        u32 random = 0;\n' + write)
mutate('double-random-call', resolve + random + '        (void)rand();\n' + write)
mutate('missing-random-byte-mask', resolve + '        u32 random = (u32)rand();\n' + write)
mutate('signed-range', resolve + random + write.replace('((u8*)operands)[1]', '(s32)((s8*)operands)[1]'))
mutate('wrong-scale-shift', resolve + random + write.replace('>> 8', '>> 7'))
mutate('wrong-range-offset', resolve + random + write.replace('operands)[1]', 'operands)[0]'))
mutate('late-destination-resolution', random + resolve + write, expected='seed-operand')
mutate('early-range-read', resolve + '        u32 range = ((u8*)operands)[1];\n' + random +
       '        *destination = (u8)((random * range) >> 8);', expected='seed-operand')
mutate('wide-output-store', resolve + random +
       '        AnimationWrite16(destination, (u8)((random * ((u8*)operands)[1]) >> 8));')
helper_start = source.index('void* func_8001FBA4(')
helper_end = source.index('\n}\n', helper_start) + len('\n}\n')
helper = source[helper_start:helper_end]
assert helper.count('(s8)pData[0x8C]') == 1 and helper.count('void* ') == 1
save('unsigned-stack-cursor', source[:helper_start] +
     helper.replace('(s8)pData[0x8C]', '(u8)pData[0x8C]') + source[helper_end:], compat)
rand_start = compat.index('\nint rand(void)\n')
rand_end = compat.index('\n}\n', rand_start) + 3
rand_body = compat[rand_start:rand_end]
assert rand_body.count('UINT32_C(0x3039)') == 1
save('wrong-rng-state', source, compat[:rand_start] +
     rand_body.replace('UINT32_C(0x3039)', 'UINT32_C(0x303A)') + compat[rand_end:])
assert len(controls) == 11
(out / 'controls.json').write_text(json.dumps(controls, indent=2) + '\n')
(out / 'controls.tsv').write_text(''.join(c['name'] + '\t' + c['expected'] + '\n' for c in controls))
PY

while IFS=$'\t' read -r name expected; do
    build_case "$name" "$TEST_OUT/$name.c" O2 "$TEST_OUT/$name.compat.c"
    status=0
    "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
    python3 - "$name" "$expected" "$status" <<'PY'
from pathlib import Path
import os, re, sys
name, expected, status = sys.argv[1:]
log = (Path(os.environ['TEST_OUT']) / (name + '.log')).read_text()
match = re.search(r'^SPRITE E5 FAIL case=(\d+) .*$', log, re.M)
assert int(status) == 1 and match, 'Expected observable mismatch from ' + name + ': ' + log
if expected == 'seed-operand':
    # All ordinary and fixture-only alias cases must pass before a real RNG
    # seed operand exposes the misplaced read or destination resolution.
    assert int(match[1]) >= 4980737, log
    assert re.search(r'alias=[456] ', match[0]), log
print('SPRITE E5 control rejected ' + name + ': ' + match[0])
PY
done < "$TEST_OUT/controls.tsv"
verify_sources
echo "SPRITE E5 GREEN: all three modes and 11 scratch-only controls; $TEST_OUT"
