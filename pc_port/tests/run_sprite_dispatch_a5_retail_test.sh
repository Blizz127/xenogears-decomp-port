#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_a5_retail_test.XXXXXXXX)
export TEST_OUT
BODY_SOURCE=${SPRITE_A5_SOURCE:-src/slus_006.64/system/animation_scripts.c}
export BODY_SOURCE
echo "SPRITE A5 artifacts: $TEST_OUT"

python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os, struct
out = Path(os.environ['TEST_OUT'])
body_source = Path(os.environ['BODY_SOURCE'])
image = Path('disc/SLUS_006.64').read_bytes()
assert sha256(image).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
pins = {'SLUS_006.64': sha256(image).hexdigest()}
for name, start, end, expected in [
    ('dispatch_table', 0x800183d8, 0x800185a4, 'ee73be97683cbf2affa5db0d128e5083ac4b273da3bc5c2f0bfc12bfb75ee0f8'),
    ('full_dispatcher', 0x8001fbe4, 0x80021ad8, '7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c'),
    ('a5_handler', 0x80021884, 0x800218dc, '2be4ca1faf57ee5d4a276767462f49d9af8f6eb82a713c7adf728ae004995fb3'),
    ('velocity_helper', 0x80022974, 0x80022a00, 'd455d43fbd61b5080bbb67d0308a518ad0fdc974bccd76a720854439212c7fed'),
    ('trig', 0x8003f8b0, 0x8003f8e8, '9687ca637b21311f891dd33fa756f8f1c31ff274ccf92923c699b087b44e4c92')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, (name, actual)
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xa5 - 0x8a) * 4 - 0x8000f800)[0] == 0x80021884
for path in [body_source, Path('pc_port/src/battle_mips_adapter.c'),
             Path('pc_port/src/psyq_compat.c'), Path('pc_port/include_shim/psyq/libgte.h'),
             Path('src/slus_006.64/system/temp1.c'),
             Path('pc_port/src/port_compat.h'), Path('pc_port/extern/PsyCross/src/psx/LIBGTE.C'),
             Path('pc_port/extern/PsyCross/src/psx/INLINE_C.C'),
             Path('pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp'),
             Path('pc_port/extern/PsyCross/src/gte/half_float.cpp'),
             Path('pc_port/tests/sprite_dispatch_a5_retail_test.c'),
             Path('pc_port/tests/run_sprite_dispatch_a5_retail_test.sh')]:
    data = path.read_bytes()
    pins[str(path)] = sha256(data).hexdigest()
    (out / path.name).write_bytes(data)
helper_source = Path('src/slus_006.64/system/temp1.c').read_text()
helper_start = helper_source.index('s32 func_80022CAC(')
depth = 0
helper_end = None
for i in range(helper_source.index('{', helper_start), len(helper_source)):
    if helper_source[i] == '{': depth += 1
    elif helper_source[i] == '}':
        depth -= 1
        if depth == 0:
            helper_end = i + 1
            break
assert helper_end is not None
helper = helper_source[helper_start:helper_end]
(out / 'a5_helper.c').write_text(
    '#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef int32_t s32; typedef int64_t s64;\n' + helper + '\n')
pins['helper_source_sha256'] = sha256(helper.encode()).hexdigest()
pins['scope'] = ('Full retail dispatcher/A5/22974/trig oracle and native existing helper; finite independent '
                 'operand/p82/angle/divisor/factor axes, whole fixture, velocity side effects and aliases; no rendering claim.')
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 opt=$2 body_source=${3:-$BODY_SOURCE}
    local -a flags common
    local compile_source="$TEST_OUT/$name.body.c"
    python3 - "$body_source" "$compile_source" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_text()
needle = 'void func_80022974(void* pSpriteData) {'
assert source.count(needle) == 1
Path(sys.argv[2]).write_text(source.replace(needle, 'void a5_velocity_impl(void* pSpriteData) {', 1))
PY
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
        -Ipc_port/extern/PsyCross/include/psx -include assert.h -include stdint.h
        -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM \
        -Ipc_port/src -c "$compile_source" -o "$TEST_OUT/$name.body.o" \
        > "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/src -c pc_port/src/psyq_compat.c \
        -o "$TEST_OUT/$name.compat.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -Ipc_port/src -c pc_port/tests/sprite_dispatch_a5_retail_test.c \
        -o "$TEST_OUT/$name.test.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -Ipc_port/src -c "$TEST_OUT/a5_helper.c" \
        -o "$TEST_OUT/$name.helper.o" >> "$TEST_OUT/$name.build.log" 2>&1
    for component in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp gte/half_float.cpp; do
        component_name=${component##*/}
        g++ -std=c++17 "${common[@]}" -fpermissive -w -include pc_port/src/port_compat.h \
            -c "pc_port/extern/PsyCross/src/$component" -o "$TEST_OUT/$name.$component_name.o" \
            >> "$TEST_OUT/$name.build.log" 2>&1
    done
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" \
        -Wl,--gc-sections pc_port/src/battle_mips_adapter.c "$TEST_OUT/$name.test.o" \
        "$TEST_OUT/$name.body.o" "$TEST_OUT/$name.compat.o" "$TEST_OUT/$name.helper.o" \
        "$TEST_OUT/$name.LIBGTE.C.o" "$TEST_OUT/$name.INLINE_C.C.o" \
        "$TEST_OUT/$name.PsyX_GTE.cpp.o" "$TEST_OUT/$name.half_float.cpp.o" \
        -lstdc++ -lm -Wl,--wrap=rcos,--wrap=rsin \
        -o "$TEST_OUT/$name.test" >> "$TEST_OUT/$name.build.log" 2>&1
}

verify_source_pins() {
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

semantic_red=0
for opt in O0 O2 UBSan; do
    if ! build_case "$opt" "$opt"; then
        cat "$TEST_OUT/$opt.build.log" >&2
        echo "SPRITE A5 INFRASTRUCTURE FAILURE: $opt" >&2
        exit 2
    fi
    status=0
    "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        cat "$TEST_OUT/$opt.log"
    elif rg -q 'sprite_animation_unimplemented.*"opcode":165' "$TEST_OUT/$opt.log"; then
        semantic_red=1
        echo "SPRITE A5 $opt SEMANTIC RED: current native dispatcher rejects opcode 0xA5" >&2
        rg 'sprite_animation_unimplemented|Assertion' "$TEST_OUT/$opt.log" >&2 || true
    else
        cat "$TEST_OUT/$opt.log" >&2
        echo "SPRITE A5 UNEXPECTED FAILURE: $opt rc=$status" >&2
        exit 1
    fi
done

verify_source_pins
if [ "$semantic_red" -ne 0 ]; then
    echo "SPRITE A5 RED: retail A5/22974/trig pins and runnable differential fixture are ready; native implementation is still unsupported" >&2
    exit 1
fi

python3 - <<'PY'
from pathlib import Path
import os
source = Path(os.environ['BODY_SOURCE']).read_text()
start = source.index('    case 0xA5:')
end = source.index('    case ', start + len('    case 0xA5:'))
body = source[start:end]
mutations = {
    'unsigned-operand': ('s32 operand = (s8)((u8*)operands)[0];',
                         's32 operand = (u8)((u8*)operands)[0];'),
    'missing-factor-increment': ('(u32)D_80059198 + 1u', '(u32)D_80059198'),
    'unsigned-p82': ('(u32)(s32)(s16)AnimationRead16(p + 0x82)',
                     '(u32)(s32)(u16)AnimationRead16(p + 0x82)'),
    'missing-negative-rounding': ('        if ((s32)product < 0) product += 0xFFFu;\n', ''),
    'wrong-fixed-shift': ('displacement = (u32)((s32)product >> 12) << 8;',
                          'displacement = (u32)((s32)product >> 12) << 4;'),
    'wrong-radius-axis': ('AnimationRead32(p + 0x18) + displacement',
                          'AnimationRead32(p + 0x10) + displacement'),
    'missing-velocity-helper': ('        func_80022974(p);\n', ''),
}
for name, (old, new) in mutations.items():
    assert body.count(old) == 1, (name, body.count(old))
    Path(os.environ['TEST_OUT'], 'control_' + name + '.c').write_text(source[:start] + body.replace(old, new, 1) + source[end:])
PY

for kind in unsigned-operand missing-factor-increment unsigned-p82 missing-negative-rounding wrong-fixed-shift wrong-radius-axis missing-velocity-helper; do
    name="control_${kind}"
    if ! build_case "$name" O2 "$TEST_OUT/$name.c"; then
        cat "$TEST_OUT/$name.build.log" >&2
        echo "SPRITE A5 CONTROL INFRASTRUCTURE FAILURE: $kind" >&2
        exit 2
    fi
    status=0
    SPRITE_A5_QUICK=1 "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ] || ! rg -q '^SPRITE A5 FAIL ' "$TEST_OUT/$name.log"; then
        cat "$TEST_OUT/$name.log" >&2
        echo "SPRITE A5 CONTROL SURVIVED: $kind" >&2
        exit 1
    fi
    echo "SPRITE A5 control rejected $kind: $(rg -m1 '^SPRITE A5 FAIL ' "$TEST_OUT/$name.log")"
done

verify_source_pins
echo "SPRITE A5 GREEN: O0/O2/UBSan bounded census and seven isolated controls; $TEST_OUT"
