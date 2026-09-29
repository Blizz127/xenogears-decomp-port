#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_f1_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE F1 artifacts: $TEST_OUT"

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os, struct

out = Path(os.environ['TEST_OUT'])
image = Path('disc/SLUS_006.64').read_bytes()
assert sha256(image).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
pins = {'SLUS_006.64': sha256(image).hexdigest()}
for name, start, end, expected in [
    ('dispatch_table', 0x800183d8, 0x800185a4, 'ee73be97683cbf2affa5db0d128e5083ac4b273da3bc5c2f0bfc12bfb75ee0f8'),
    ('full_dispatcher', 0x8001fbe4, 0x80021ad8, '7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c'),
    ('f1_handler', 0x80020f4c, 0x80020fc8, '5586e29f8450e0f8212f2c8972a12485a0c1074ecdd0b9633d6642a7d3d26ed3'),
    ('leaf', 0x8001f6b0, 0x8001f750, 'cb7dff828c141c1959ce71eba1492665e5bdf561cc0f7a9859ed08ac6efe2884')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xf1 - 0x8a) * 4 - 0x8000f800)[0] == 0x80020f4c

for path in [
    'src/slus_006.64/system/animation_scripts.c',
    'src/slus_006.64/system/rendering.c',
    'pc_port/src/battle_mips_adapter.c', 'pc_port/src/psyq_compat.c',
    'pc_port/include_shim/psyq/libgte.h', 'pc_port/src/port_compat.h',
    'pc_port/extern/PsyCross/src/psx/LIBGTE.C',
    'pc_port/extern/PsyCross/src/psx/INLINE_C.C',
    'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp',
    'pc_port/extern/PsyCross/src/gte/half_float.cpp',
    'pc_port/tests/sprite_dispatch_f1_retail_test.c',
    'pc_port/tests/run_sprite_dispatch_f1_retail_test.sh']:
    data = Path(path).read_bytes()
    pins[path] = sha256(data).hexdigest()
    (out / Path(path).name).write_bytes(data)

rendering = Path('src/slus_006.64/system/rendering.c').read_text()
start = rendering.index('void func_8001F6B0(void* pSpriteData)')
end = rendering.index('\n}\n', start) + 3
leaf = rendering[start:end]
assert leaf.count('void func_8001F6B0(') == 1
leaf_impl = leaf.replace('void func_8001F6B0(', 'void f1_leaf_impl(', 1)
leaf_source = ('#include <stdint.h>\n'
               'typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;\n'
               + leaf_impl)
(out / 'f1_leaf.c').write_text(leaf_source)
pins['f1_leaf_source_sha256'] = sha256(leaf.encode()).hexdigest()
pins['scope'] = ('Exact retail F1 dispatcher and verbatim rendering leaf; operand read order, '
                 'base pointer snapshot, mode-2 halfword stores, mode-1 leaf callback, '
                 'finite byte edge cross and alias census; no exhaustive 16M RGB cross product claim.')
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 opt=$2 body_source=${3:-src/slus_006.64/system/animation_scripts.c}
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -c "$body_source" -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -DUSE_EXTENDED_PRIM_POINTERS=0 -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/src -Ipc_port/include_shim -Iinclude -c pc_port/src/psyq_compat.c -o "$TEST_OUT/$name.compat.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -Ipc_port/src -c "$TEST_OUT/f1_leaf.c" -o "$TEST_OUT/$name.leaf.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -Ipc_port/src -c pc_port/tests/sprite_dispatch_f1_retail_test.c -o "$TEST_OUT/$name.test.o" >> "$TEST_OUT/$name.build.log" 2>&1
    for component in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp gte/half_float.cpp; do
        component_name=${component##*/}
        g++ -std=c++17 "${common[@]}" -fpermissive -w -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$component" -o "$TEST_OUT/$name.$component_name.o" >> "$TEST_OUT/$name.build.log" 2>&1
    done
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections "$TEST_OUT/$name.test.o" pc_port/src/battle_mips_adapter.c "$TEST_OUT/$name.body.o" "$TEST_OUT/$name.compat.o" "$TEST_OUT/$name.leaf.o" "$TEST_OUT/$name.LIBGTE.C.o" "$TEST_OUT/$name.INLINE_C.C.o" "$TEST_OUT/$name.PsyX_GTE.cpp.o" "$TEST_OUT/$name.half_float.cpp.o" -lstdc++ -lm -o "$TEST_OUT/$name.test" >> "$TEST_OUT/$name.build.log" 2>&1
}

verify_source_pins() {
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path:
        assert sha256(Path(path).read_bytes()).hexdigest() == expected, 'Source changed during test: ' + path
PY
}

semantic_red=0
for opt in O0 O2 UBSan; do
    if ! build_case "$opt" "$opt"; then
        cat "$TEST_OUT/$opt.build.log" >&2
        echo "SPRITE F1 INFRASTRUCTURE FAILURE: $opt" >&2
        exit 2
    fi
    status=0
    "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        cat "$TEST_OUT/$opt.log"
    elif rg -q 'sprite_animation_unimplemented.*"opcode":241' "$TEST_OUT/$opt.log"; then
        semantic_red=1
        echo "SPRITE F1 $opt SEMANTIC RED: current native dispatcher rejects opcode 0xF1" >&2
        rg 'sprite_animation_unimplemented|Assertion' "$TEST_OUT/$opt.log" >&2 || true
    else
        cat "$TEST_OUT/$opt.log" >&2
        echo "SPRITE F1 UNEXPECTED FAILURE: $opt rc=$status" >&2
        exit 1
    fi
done

verify_source_pins
if [ "$semantic_red" -ne 0 ]; then
    echo "SPRITE F1 RED: retail F1 pins and one-case native-vs-retail fixture are ready" >&2
    exit 1
fi
python3 - <<'MUTANTS'
from pathlib import Path
import os, json
out=Path(os.environ['TEST_OUT'])
source=Path('src/slus_006.64/system/animation_scripts.c').read_text()
start=source.index('    case 0xF1: {')
end=source.index('    case 0x91:',start)
body=source[start:end]
mutants={
 'wrong-red-field': body.replace('p[0x28] = red;', 'p[0x29] = red;'),
 'signed-model-red': body.replace('base + 0x38, source[0]', 'base + 0x38, (s8)source[0]'),
 'wrong-mode': body.replace('(flags & 3u) == 2u', '(flags & 3u) == 3u'),
 'missing-leaf': body.replace('            func_8001F6B0(p);', '            (void)p;'),
 'cached-red': body.replace('base + 0x38, source[0]', 'base + 0x38, red'),
 'cached-final-mode': body.replace('(AnimationRead32(p + 0x3C) & 3u) == 1u', '(flags & 3u) == 1u'),
 'neighbor-corruption': body.replace('        return;', '        p[0x2B] = 0;\n        return;'),
 'wrong-blue': body.replace('p[0x2A] = source[2];', 'p[0x2A] = source[1];'),
 'early-green-read': body.replace('u8 red = source[0];', 'u8 red = source[0];\n        u8 green = source[1];').replace('p[0x29] = source[1];', 'p[0x29] = green;'),
}
for name,changed in mutants.items():
 assert changed != body, name
 (out/(name+'.c')).write_text(source[:start]+changed+source[end:])
(out/'controls.json').write_text(json.dumps(list(mutants),indent=2)+'\n')
(out/'controls.txt').write_text('\n'.join(mutants)+'\n')
MUTANTS
while IFS= read -r name; do
    build_case "$name" O2 "$TEST_OUT/$name.c"
    status=0
    "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
    if [ "$status" -ne 1 ] || ! rg -q '^SPRITE F1 FAIL case=' "$TEST_OUT/$name.log"; then
        cat "$TEST_OUT/$name.log" >&2
        echo "SPRITE F1 control failed to reject semantically: $name rc=$status" >&2
        exit 1
    fi
    echo "SPRITE F1 control rejected $name"
done < "$TEST_OUT/controls.txt"
verify_source_pins
python3 - <<'RESULT'
from pathlib import Path
import os,json,re
out=Path(os.environ['TEST_OUT']);result={'modes':{},'controls':[]}
for mode in ['O0','O2','UBSan']:
 match=re.search(r'SPRITE F1 PASS (\d+) cases', (out/(mode+'.log')).read_text());assert match
 result['modes'][mode]={'cases':int(match[1])}
for name in json.loads((out/'controls.json').read_text()):
 match=re.search(r'^SPRITE F1 FAIL case=.*$',(out/(name+'.log')).read_text(),re.M);assert match
 result['controls'].append({'name':name,'mismatch':match[0]})
(out/'result-summary.json').write_text(json.dumps(result,indent=2)+'\n')
RESULT
echo "SPRITE F1 GREEN: O0/O2/UBSan finite native-vs-retail census and nine controls; $TEST_OUT"
