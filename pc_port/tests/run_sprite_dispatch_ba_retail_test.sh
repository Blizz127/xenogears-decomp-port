#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_ba_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE BA artifacts: $TEST_OUT"

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os, struct

out = Path(os.environ['TEST_OUT'])
image = Path('disc/SLUS_006.64').read_bytes()
assert sha256(image).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
pins = {'SLUS_006.64': sha256(image).hexdigest()}
for name, start, end, expected in [
    ('dispatch_table', 0x800183d8, 0x800185a4,
     'ee73be97683cbf2affa5db0d128e5083ac4b273da3bc5c2f0bfc12bfb75ee0f8'),
    ('full_dispatcher', 0x8001fbe4, 0x80021ad8,
     '7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c'),
    ('ba_handler', 0x80020f38, 0x80020f4c,
     '2c65e8fb3a9e202375f8ff8b7ecddd7bef9d0ca8bd85cd1a6b106e6d3744d183'),
    ('ba_helper', 0x80023290, 0x80023340,
     '3bc9b41f66197683ac7c79538a510ced016b5c05d9bb5877906cac6e743eabbe'),
    ('leaf', 0x8001f6b0, 0x8001f750,
     'cb7dff828c141c1959ce71eba1492665e5bdf561cc0f7a9859ed08ac6efe2884')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image,
                         0x800183d8 + (0xBA - 0x8A) * 4 - 0x8000f800)[0] == 0x80020f38

for path in [
    'src/slus_006.64/system/animation_scripts.c',
    'src/slus_006.64/system/temp1.c',
    'pc_port/src/battle_mips_adapter.c',
    'pc_port/src/psyq_compat.c',
    'pc_port/include_shim/psyq/libgte.h',
    'pc_port/src/port_compat.h',
    'src/slus_006.64/system/rendering.c',
    'pc_port/extern/PsyCross/src/psx/LIBGTE.C',
    'pc_port/extern/PsyCross/src/psx/INLINE_C.C',
    'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp',
    'pc_port/extern/PsyCross/src/gte/half_float.cpp',
    'pc_port/tests/sprite_dispatch_ba_retail_test.c',
    'pc_port/tests/run_sprite_dispatch_ba_retail_test.sh']:
    data = Path(path).read_bytes()
    pins[path] = sha256(data).hexdigest()
    (out / Path(path).name).write_bytes(data)

source = Path('src/slus_006.64/system/temp1.c').read_text()
start = source.index('void func_80023290(u8* pSprite, s32 animType)')
end = source.index('\n}\n', start) + 3
helper = source[start:end]
assert helper.count('void func_80023290(') == 1
helper_impl = helper.replace('void func_80023290(', 'void ba_helper_impl(', 1)
helper_source = (
    '#include <stdint.h>\n'
    'typedef uint8_t u8; typedef uint32_t u32; typedef int32_t s32;\n'
    'extern void func_8001F6B0(void *);\n' + helper_impl)
(out / 'ba_helper.c').write_text(helper_source)
pins['ba_helper_source_sha256'] = sha256(helper.encode()).hexdigest()
rendering = Path('src/slus_006.64/system/rendering.c').read_text()
leaf_start = rendering.index('void func_8001F6B0(void* pSpriteData)')
leaf_end = rendering.index('\n}\n', leaf_start) + 3
leaf = rendering[leaf_start:leaf_end]
assert leaf.count('void func_8001F6B0(') == 1
leaf_impl = leaf.replace('void func_8001F6B0(', 'void ba_leaf_impl(', 1)
leaf_source = (
    '#include <stdint.h>\n'
    'typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;\n'
    + leaf_impl)
(out / 'ba_leaf.c').write_text(leaf_source)
pins['ba_leaf_source_sha256'] = sha256(leaf.encode()).hexdigest()
pins['scope'] = (
    'One retail/native BA dispatcher fixture plus verbatim native helper body; '
    'helper argument/order, verbatim leaf effects, and strict 8001F6B0 entry '
    'observation. This bootstrap RED does not claim exhaustive input coverage '
    'or displayed rendering.'
)
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 opt=$2 body_source=${3:-src/slus_006.64/system/animation_scripts.c}
    local helper_source=${4:-$TEST_OUT/ba_helper.c}
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    common=(-Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
            -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie
            -ffunction-sections -fdata-sections "${flags[@]}")
    gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM \
        -Ipc_port/include_shim -Iinclude -c "$body_source" \
        -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/src \
        -c pc_port/src/psyq_compat.c -o "$TEST_OUT/$name.compat.o" \
        >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -Ipc_port/include_shim -Iinclude \
        -Ipc_port/src -c "$helper_source" -o "$TEST_OUT/$name.helper.o" \
        >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -Ipc_port/include_shim -Iinclude \
        -Ipc_port/src -c "$TEST_OUT/ba_leaf.c" -o "$TEST_OUT/$name.leaf.o" \
        >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -Ipc_port/src \
        -c pc_port/tests/sprite_dispatch_ba_retail_test.c \
        -o "$TEST_OUT/$name.test.o" >> "$TEST_OUT/$name.build.log" 2>&1
    for component in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp gte/half_float.cpp; do
        component_name=${component##*/}
        g++ -std=c++17 "${common[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h \
            -c "pc_port/extern/PsyCross/src/$component" \
            -o "$TEST_OUT/$name.$component_name.o" >> "$TEST_OUT/$name.build.log" 2>&1
    done
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" \
        -Wl,--gc-sections "$TEST_OUT/$name.test.o" pc_port/src/battle_mips_adapter.c \
        "$TEST_OUT/$name.body.o" "$TEST_OUT/$name.compat.o" "$TEST_OUT/$name.helper.o" \
        "$TEST_OUT/$name.leaf.o" \
        "$TEST_OUT/$name.LIBGTE.C.o" "$TEST_OUT/$name.INLINE_C.C.o" \
        "$TEST_OUT/$name.PsyX_GTE.cpp.o" "$TEST_OUT/$name.half_float.cpp.o" \
        -lstdc++ -lm -Wl,--wrap=rand -o "$TEST_OUT/$name.test" \
        >> "$TEST_OUT/$name.build.log" 2>&1
}

make_dispatch_mutant() {
    local kind=$1 output="$TEST_OUT/animation_scripts.$1.c"
    python3 - "$kind" src/slus_006.64/system/animation_scripts.c "$output" <<'PY'
from pathlib import Path
import sys
kind, source_name, output_name = sys.argv[1:]
text = Path(source_name).read_text()
start = text.index('    case 0xBA:')
end = text.index('    case ', start + 1)
body = text[start:end]
mutations = {
    'wrong-operand': (
        '((u8*)operands)[0]', '((u8*)operands)[1]'),
    'wrong-sprite': (
        '(u8*)pSpriteData', '(u8*)pSpriteData + 1'),
    'missing-helper': (
        'func_80023290((u8*)pSpriteData, ((u8*)operands)[0]);',
        '(void)0;'),
    'wrong-helper-arg': (
        '((u8*)operands)[0]);', '((u8*)operands)[0] + 1);'),
}
old, new = mutations[kind]
assert body.count(old) == 1, (kind, body.count(old))
Path(output_name).write_text(text[:start] + body.replace(old, new) + text[end:])
PY
    printf '%s\n' "$output"
}

make_helper_mutant() {
    local kind=$1 output="$TEST_OUT/ba_helper.$1.c"
    python3 - "$kind" "$TEST_OUT/ba_helper.c" "$output" <<'PY'
from pathlib import Path
import sys
kind, source_name, output_name = sys.argv[1:]
text = Path(source_name).read_text()
mutations = {
    'wrong-mask': ('animType &= 7;', 'animType &= 3;'),
    'wrong-bit-set': ('(animType << 5)', '(animType << 4)'),
    'wrong-type-gate': ('if (type == 8 || type == 9)', 'if (type == 8)'),
}
old, new = mutations[kind]
assert text.count(old) == 1, (kind, text.count(old))
Path(output_name).write_text(text.replace(old, new))
PY
    printf '%s\n' "$output"
}

verify_source_pins() {
    python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path:
        assert sha256(Path(path).read_bytes()).hexdigest() == expected, \
            'Source changed during test: ' + path
PY
}

for opt in O0 O2 UBSan; do
    build_case "$opt" "$opt"
done

failed=0
for opt in O0 O2 UBSan; do
    status=0
    "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -ne 0 ]; then
        cat "$TEST_OUT/$opt.log" >&2
        echo "SPRITE BA $opt FAILURE rc=$status" >&2
        failed=1
    else
        cat "$TEST_OUT/$opt.log"
    fi
done
if [ "$failed" -ne 0 ]; then
    verify_source_pins
    echo "SPRITE BA RED: at least one O0/O2/UBSan finite census failed" >&2
    exit 1
fi

run_control() {
    local kind=$1 source=$2 helper=${3:-$TEST_OUT/ba_helper.c}
    build_case "control_$kind" O2 "$source" "$helper"
    local status=0
    local quick_env=(SPRITE_BA_QUICK=1)
    if [ "$kind" = wrong-type-gate ]; then
        quick_env+=(SPRITE_BA_QUICK_TYPE=9)
    fi
    env "${quick_env[@]}" "$TEST_OUT/control_$kind.test" \
        > "$TEST_OUT/control_$kind.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ] || ! rg -q '^SPRITE BA FAIL case=' "$TEST_OUT/control_$kind.log"; then
        cat "$TEST_OUT/control_$kind.log" >&2
        echo "SPRITE BA CONTROL SURVIVED: $kind" >&2
        exit 1
    fi
    echo "SPRITE BA control rejected $kind: $(rg -m1 '^SPRITE BA FAIL case=' "$TEST_OUT/control_$kind.log")"
}

for kind in wrong-operand wrong-sprite missing-helper wrong-helper-arg; do
    mutant=$(make_dispatch_mutant "$kind")
    run_control "$kind" "$mutant"
done
for kind in wrong-mask wrong-bit-set wrong-type-gate; do
    mutant=$(make_helper_mutant "$kind")
    run_control "$kind" src/slus_006.64/system/animation_scripts.c "$mutant"
done

verify_source_pins
echo "SPRITE BA GREEN: O0/O2/UBSan finite census plus 7 isolated controls; $TEST_OUT"
