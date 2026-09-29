#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_91_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE 91 artifacts: $TEST_OUT"

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
    ('handler', 0x80020de8, 0x80020df8, '61dbe547fd59b86996a85bbb8989f581efb69840700618d08aa29415ca4f7f53'),
    ('common', 0x80020e04, 0x80020e14, 'c3dcc69293dfd4397e726fa5ab61ea8acb501a346ea16daffe31e7eb12d32560'),
    ('leaf', 0x8001f6b0, 0x8001f750, 'cb7dff828c141c1959ce71eba1492665e5bdf561cc0f7a9859ed08ac6efe2884')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0x91 - 0x8a) * 4 - 0x8000f800)[0] == 0x80020de8
for path in ['src/slus_006.64/system/animation_scripts.c', 'src/slus_006.64/system/rendering.c',
             'pc_port/src/battle_mips_adapter.c', 'pc_port/src/psyq_compat.c',
             'pc_port/include_shim/psyq/libgte.h', 'pc_port/src/port_compat.h',
             'pc_port/extern/PsyCross/src/psx/LIBGTE.C', 'pc_port/extern/PsyCross/src/psx/INLINE_C.C',
             'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp', 'pc_port/extern/PsyCross/src/gte/half_float.cpp',
             'pc_port/tests/sprite_dispatch_91_retail_test.c', 'pc_port/tests/run_sprite_dispatch_91_retail_test.sh']:
    data = Path(path).read_bytes(); pins[path] = sha256(data).hexdigest(); (out / Path(path).name).write_bytes(data)
source = Path('src/slus_006.64/system/rendering.c').read_text()
start = source.index('void func_8001F6B0(void* pSpriteData)'); end = source.index('\n}\n', start) + 3
leaf = source[start:end]
assert leaf.count('void func_8001F6B0(') == 1
(out / 'ba_leaf.c').write_text('#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;\n' + leaf.replace('void func_8001F6B0(', 'void ba_leaf_impl(', 1))
pins['leaf_source_sha256'] = sha256(leaf.encode()).hexdigest()
pins['scope'] = 'One 0x91 retail/native dispatcher fixture with NULL operands, bit clear, actual retail/native leaf effects, frame backing, and handler/common/leaf call order; no displayed-rendering claim.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 opt=$2 body_source=${3:-src/slus_006.64/system/animation_scripts.c}
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -c "$body_source" -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -DUSE_EXTENDED_PRIM_POINTERS=0 -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/src -c pc_port/src/psyq_compat.c -o "$TEST_OUT/$name.compat.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -Ipc_port/include_shim -Iinclude -Ipc_port/src -c "$TEST_OUT/ba_leaf.c" -o "$TEST_OUT/$name.leaf.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -Ipc_port/src -c pc_port/tests/sprite_dispatch_91_retail_test.c -o "$TEST_OUT/$name.test.o" >> "$TEST_OUT/$name.build.log" 2>&1
    for component in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp gte/half_float.cpp; do
        component_name=${component##*/}
        g++ -std=c++17 "${common[@]}" -fpermissive -w -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$component" -o "$TEST_OUT/$name.$component_name.o" >> "$TEST_OUT/$name.build.log" 2>&1
    done
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections "$TEST_OUT/$name.test.o" pc_port/src/battle_mips_adapter.c "$TEST_OUT/$name.body.o" "$TEST_OUT/$name.compat.o" "$TEST_OUT/$name.leaf.o" "$TEST_OUT/$name.LIBGTE.C.o" "$TEST_OUT/$name.INLINE_C.C.o" "$TEST_OUT/$name.PsyX_GTE.cpp.o" "$TEST_OUT/$name.half_float.cpp.o" -lstdc++ -lm -Wl,--wrap=rand -o "$TEST_OUT/$name.test" >> "$TEST_OUT/$name.build.log" 2>&1
}

make_mutant() {
    local kind=$1 output="$TEST_OUT/animation_scripts.$1.c"
    python3 - "$kind" src/slus_006.64/system/animation_scripts.c "$output" <<'PY'
from pathlib import Path
import sys
kind, src, dst = sys.argv[1:]
text = Path(src).read_text()
start = text.index('    case 0x91:')
end = text.index('    case ', start + 1)
body = text[start:end]
mutations = {
    'no-bit-clear': ('((u8*)pSpriteData)[0x2B] &= 0xFEu;', '(void)0;'),
    'wrong-bit': ('((u8*)pSpriteData)[0x2B] &= 0xFEu;', '((u8*)pSpriteData)[0x2B] &= 0xFDu;'),
    'callee-before-store': (
        '((u8*)pSpriteData)[0x2B] &= 0xFEu;\n        func_8001F6B0(pSpriteData);',
        'func_8001F6B0(pSpriteData);\n        ((u8*)pSpriteData)[0x2B] &= 0xFEu;'),
    'missing-leaf': ('func_8001F6B0(pSpriteData);', '(void)0;'),
    'corrupt-neighbor': ('((u8*)pSpriteData)[0x2B] &= 0xFEu;', '((u8*)pSpriteData)[0x2A] &= 0xFEu;'),
}
old, new = mutations[kind]
assert body.count(old) == 1, (kind, body.count(old), body)
Path(dst).write_text(text[:start] + body.replace(old, new) + text[end:])
PY
    printf '%s\n' "$output"
}

run_control() {
    local kind=$1 source=$2
    build_case "control_$kind" O2 "$source"
    local status=0
    SPRITE_91_QUICK=1 "$TEST_OUT/control_$kind.test" > "$TEST_OUT/control_$kind.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ] || ! rg -q '^SPRITE 91 FAIL ' "$TEST_OUT/control_$kind.log"; then
        cat "$TEST_OUT/control_$kind.log" >&2
        echo "SPRITE 91 CONTROL SURVIVED: $kind" >&2
        exit 1
    fi
    echo "SPRITE 91 control rejected $kind: $(rg -m1 '^SPRITE 91 FAIL ' "$TEST_OUT/control_$kind.log")"
}

if [ "${SPRITE_91_RUN_GREEN:-1}" = 1 ]; then
    for opt in O0 O2 UBSan; do
        build_case "$opt" "$opt"
        "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1
        cat "$TEST_OUT/$opt.log"
    done
    for kind in no-bit-clear wrong-bit callee-before-store missing-leaf corrupt-neighbor; do
        run_control "$kind" "$(make_mutant "$kind")"
    done
    python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
pins=json.loads((Path(os.environ['TEST_OUT'])/'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path: assert sha256(Path(path).read_bytes()).hexdigest()==expected, 'Source changed during test: '+path
PY
    echo "SPRITE 91 GREEN: O0/O2/UBSan exhaustive census plus 5 isolated controls; $TEST_OUT"
    exit 0
fi

for opt in O0 O2 UBSan; do
    build_case "$opt" "$opt"
    status=0
    "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ] || ! rg -q 'sprite_animation_unimplemented.*"opcode":145' "$TEST_OUT/$opt.log"; then
        cat "$TEST_OUT/$opt.log" >&2
        echo "SPRITE 91 UNEXPECTED RESULT $opt rc=$status" >&2
        exit 1
    fi
    echo "SPRITE 91 $opt SEMANTIC RED: native dispatcher rejects opcode 0x91"
done

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path:
        assert sha256(Path(path).read_bytes()).hexdigest() == expected, 'Source changed during test: ' + path
PY
echo "SPRITE 91 RED: one NULL-operand retail fixture and leaf source pins are runnable; native 0x91 remains unsupported"
