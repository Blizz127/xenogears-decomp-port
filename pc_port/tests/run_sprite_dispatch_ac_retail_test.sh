#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_ac_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE AC artifacts: $TEST_OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os, struct
out = Path(os.environ['TEST_OUT']); image = Path('disc/SLUS_006.64').read_bytes()
assert sha256(image).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
pins = {'SLUS_006.64': sha256(image).hexdigest()}
for name, start, end, expected in [
    ('dispatch_table', 0x800183d8, 0x800185a4, 'ee73be97683cbf2affa5db0d128e5083ac4b273da3bc5c2f0bfc12bfb75ee0f8'),
    ('full_dispatcher', 0x8001fbe4, 0x80021ad8, '7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c'),
    ('ac_handler', 0x80021644, 0x80021698, 'dc54f9d50528ab17f3a9cc0c8813fb1743ced8ceac92d383d2044ff8448e1620'),
    ('angle_helper', 0x80021fe0, 0x80022000, 'ae7dedf42d5581e07867862da9b6e43bafe1a11110d44ce68c81a8ee6f522411'),
    ('velocity_helper', 0x80022974, 0x80022a00, 'd455d43fbd61b5080bbb67d0308a518ad0fdc974bccd76a720854439212c7fed'),
    ('rand', 0x8003fa38, 0x8003fa68, '8efec2e9f765b1fe90da7f814c3864d0a4c87d4df7e4bc67af491d5a30c21e81')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest(); assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xac - 0x8a) * 4 - 0x8000f800)[0] == 0x80021644
for path in ['src/slus_006.64/system/animation_scripts.c', 'pc_port/src/battle_mips_adapter.c', 'pc_port/src/psyq_compat.c',
             'pc_port/include_shim/psyq/libgte.h', 'pc_port/extern/PsyCross/src/psx/LIBGTE.C',
             'pc_port/extern/PsyCross/src/psx/INLINE_C.C', 'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp',
             'pc_port/extern/PsyCross/src/gte/half_float.cpp',
             'pc_port/tests/sprite_dispatch_ac_retail_test.c', 'pc_port/tests/run_sprite_dispatch_ac_retail_test.sh']:
    data = Path(path).read_bytes(); pins[path] = sha256(data).hexdigest(); (out / Path(path).name).write_bytes(data)
pins['scope'] = 'Full native/retail AC dispatcher and helper differential; all random-byte/range pairs, all masked angles, divisor zero..4095, radius boundaries, operand/seed aliases, whole fixture and RNG state.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 opt=$2 source=${3:-src/slus_006.64/system/animation_scripts.c}; local -a flags common
    flags=(-"$opt"); if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -c "$source" -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1 || { cat "$TEST_OUT/$name.build.log" >&2; return 2; }
    gcc -std=gnu17 -fpermissive "${common[@]}" -DUSE_EXTENDED_PRIM_POINTERS=0 -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/src -c pc_port/src/psyq_compat.c -o "$TEST_OUT/$name.compat.o" >> "$TEST_OUT/$name.build.log" 2>&1 || { cat "$TEST_OUT/$name.build.log" >&2; return 2; }
    for component in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp gte/half_float.cpp; do
        component_name=${component##*/}; g++ -std=c++17 "${common[@]}" -fpermissive -w -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$component" -o "$TEST_OUT/$name.$component_name.o" >> "$TEST_OUT/$name.build.log" 2>&1 || { cat "$TEST_OUT/$name.build.log" >&2; return 2; }
    done
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_ac_retail_test.c pc_port/src/battle_mips_adapter.c "$TEST_OUT/$name.body.o" "$TEST_OUT/$name.compat.o" "$TEST_OUT/$name.LIBGTE.C.o" "$TEST_OUT/$name.INLINE_C.C.o" "$TEST_OUT/$name.PsyX_GTE.cpp.o" "$TEST_OUT/$name.half_float.cpp.o" -lstdc++ -lm -Wl,--wrap=rand -o "$TEST_OUT/$name.test" >> "$TEST_OUT/$name.build.log" 2>&1 || { cat "$TEST_OUT/$name.build.log" >&2; return 2; }
}
failed=0
if [ -z "${SPRITE_AC_ONLY_CONTROL:-}" ]; then
    for opt in O0 O2 UBSan; do
        build_case "$opt" "$opt"
        if "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1; then cat "$TEST_OUT/$opt.log"; else cat "$TEST_OUT/$opt.log" >&2; failed=1; fi
    done
fi
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path: assert sha256(Path(path).read_bytes()).hexdigest() == expected, 'Source changed during test: ' + path
PY
if [ "$failed" != 0 ]; then echo "SPRITE AC RED: see $TEST_OUT (source snapshots, retail pins and build logs)" >&2; exit 1; fi
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
out = Path(os.environ['TEST_OUT'])
source = (out / 'animation_scripts.c').read_text()
ac_start = source.index('    case 0xAC:')
ac_end = source.index('    case ', ac_start + len('    case 0xAC:'))
ac_body = source[ac_start:ac_end]
controls = []

def save(name, changed):
    assert changed != source
    path = out / (name + '.c')
    path.write_text(changed)
    controls.append({'name': name, 'sha256': sha256(changed.encode()).hexdigest()})

def ac_mutate(name, old, new):
    assert ac_body.count(old) == 1, (name, old, ac_body.count(old))
    save(name, source[:ac_start] + ac_body.replace(old, new) + source[ac_end:])

ac_mutate('wrong-rng-order',
          '        u32 randomByte = (u32)rand() & 0xFFu;\n'
          '        /* 8002164C reads after RNG, even when operands alias its seed. */\n'
          '        u32 range = ((u8*)operands)[0];',
          '        u32 range = ((u8*)operands)[0];\n'
          '        u32 randomByte = (u32)rand() & 0xFFu;')
ac_mutate('missing-rng-mask', '(u32)rand() & 0xFFu', '(u32)rand()')
ac_mutate('signed-range', 'u32 range = ((u8*)operands)[0];',
          's32 range = (s32)(s8)((u8*)operands)[0];')
ac_mutate('wrong-range-centering', 'range >> 1', 'range')
ac_mutate('wrong-scale', 'delta * 16', 'delta * 8')
ac_mutate('wrong-angle-field', 'AnimationRead16(p + 0x32)', 'AnimationRead16(p + 0x30)')
ac_mutate('unrelated-store', '        return;\n    }',
          '        AnimationWrite32(p + 0x10, 0);\n        return;\n    }')
ac_mutate('double-rng', '        u32 randomByte = (u32)rand() & 0xFFu;',
          '        u32 randomByte = (u32)rand() & 0xFFu;\n        (void)rand();')

helper_need = '                              : ((s32)radius < 0 ? 1u : 0xFFFFFFFFu);'
assert source.count(helper_need) == 1
save('wrong-divisor-zero', source.replace(helper_need, '                              : 0u;'))
assert source.count('angle & 0xFFFu') == 2
save('wrong-trig-mask', source.replace('angle & 0xFFFu', 'angle & 0x7FFu', 2))
old = ('product = 0u - (u32)trigValue * scaledRadius;\n'
       '    AnimationWrite32(pData + 0x14, (u32)((s32)product >> 6));')
new = ('product = (u32)trigValue * scaledRadius;\n'
       '    AnimationWrite32(pData + 0x14, 0u - (u32)((s32)product >> 6));')
assert source.count(old) == 1
save('wrong-negation-order', source.replace(old, new))
old = '(u32)((s32)product >> 6)'
assert source.count(old) == 2
save('logical-output-shift', source.replace(old, 'product >> 6'))
(out / 'controls.json').write_text(json.dumps(controls, indent=2) + '\n')
(out / 'controls.tsv').write_text(''.join(c['name'] + '\n' for c in controls))
assert len(controls) == 12
PY

while IFS= read -r name; do
    if [ -n "${SPRITE_AC_ONLY_CONTROL:-}" ] && [ "$name" != "$SPRITE_AC_ONLY_CONTROL" ]; then continue; fi
    build_case "$name" O2 "$TEST_OUT/$name.c"
    status=0
    SPRITE_AC_QUICK=1 "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
    python3 - "$name" "$status" <<'PY'
from pathlib import Path
import os, re, sys
name, status = sys.argv[1:]
log = (Path(os.environ['TEST_OUT']) / (name + '.log')).read_text()
match = re.search(r'^SPRITE AC FAIL case=(\d+) .*$', log, re.M)
assert int(status) == 1 and match, 'Expected observable mismatch from ' + name + ': ' + log
print('SPRITE AC control rejected ' + name + ': ' + match.group(0))
PY
done < "$TEST_OUT/controls.tsv"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path: assert sha256(Path(path).read_bytes()).hexdigest() == expected, 'Source changed during test: ' + path
PY
python3 - <<'PY'
from pathlib import Path
import json, os, re
out = Path(os.environ['TEST_OUT'])
summary = {'baseline_modes': {}, 'controls': []}
for mode in ('O0', 'O2', 'UBSan'):
    path = out / (mode + '.log')
    if path.exists():
        text = path.read_text()
        summary['baseline_modes'][mode] = {
            'pass': 'SPRITE AC PASS 4616193 cases' in text,
            'log': path.name,
        }
for line in (out / 'controls.tsv').read_text().splitlines():
    if not line or (os.environ.get('SPRITE_AC_ONLY_CONTROL') and
                    line != os.environ['SPRITE_AC_ONLY_CONTROL']):
        continue
    path = out / (line + '.log')
    text = path.read_text() if path.exists() else ''
    match = re.search(r'^SPRITE AC FAIL case=.*$', text, re.M)
    summary['controls'].append({'name': line, 'rejected': bool(match),
                                'first_failure': match.group(0) if match else None,
                                'log': path.name})
(out / 'result-summary.json').write_text(json.dumps(summary, indent=2) + '\n')
PY
echo "SPRITE AC GREEN: full native/retail coverage and 12 scratch-only negative controls; $TEST_OUT"
