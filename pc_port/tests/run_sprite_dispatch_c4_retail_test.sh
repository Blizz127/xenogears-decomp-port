#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_c4_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE C4 artifacts: $TEST_OUT"

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
    ('c4_handler', 0x800215b8, 0x80021644, 'd7ec4084d92d84a4bf626d4041f06a7f2a180914f393757d7fd3bd4d03faa0cd'),
    ('rand', 0x8003fa38, 0x8003fa68, '8efec2e9f765b1fe90da7f814c3864d0a4c87d4df7e4bc67af491d5a30c21e81'),
    ('vector_setter', 0x80021b04, 0x80021b14, '694486283a46f5f80c141894f7830f37a26ff26f7fe6c1784666b2dc72c14488'),
    ('rot_matrix', 0x8003f738, 0x8003f8b0, '7fc22ca6e32404ca5e686638cf10ed556c897420df23d54ce7b2e4fb9b8fe9e1'),
    ('apply_matrix_lv', 0x8004947c, 0x800495dc, 'b5cca44ff50bf0ca0e5996e8f5cf1b954740710e6773d57827b764a47fee082e')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xc4 - 0x8a) * 4 - 0x8000f800)[0] == 0x800215b8
for path in [
    'src/slus_006.64/system/animation_scripts.c',
    'pc_port/src/battle_mips_adapter.c', 'pc_port/src/psyq_compat.c',
    'pc_port/include_shim/psyq/libgte.h', 'pc_port/src/port_compat.h',
    'pc_port/extern/PsyCross/src/psx/LIBGTE.C',
    'pc_port/extern/PsyCross/src/psx/INLINE_C.C',
    'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp',
    'pc_port/extern/PsyCross/src/gte/half_float.cpp',
    'pc_port/tests/sprite_dispatch_c4_retail_test.c',
    'pc_port/tests/run_sprite_dispatch_c4_retail_test.sh']:
    data = Path(path).read_bytes()
    pins[path] = sha256(data).hexdigest()
    (out / Path(path).name).write_bytes(data)
pins['scope'] = 'Full native and interpreted retail dispatcher; exact C4/RNG/vector/RotMatrix/ApplyMatrixLV retail pins; actual PsyCross GTE backend; finite velocity/range/seed/alias census; no exhaustive 32-bit claim.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 opt=$2 body_source=${3:-src/slus_006.64/system/animation_scripts.c}
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM \
        -Ipc_port/include_shim -Iinclude -c "$body_source" \
        -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -include assert.h -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/src -Ipc_port/include_shim -Iinclude \
        -c pc_port/src/psyq_compat.c -o "$TEST_OUT/$name.compat.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -Ipc_port/src -c pc_port/tests/sprite_dispatch_c4_retail_test.c \
        -o "$TEST_OUT/$name.test.o" >> "$TEST_OUT/$name.build.log" 2>&1
    for component in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp gte/half_float.cpp; do
        component_name=${component##*/}
        g++ -std=c++17 "${common[@]}" -fpermissive -w -include pc_port/src/port_compat.h \
            -c "pc_port/extern/PsyCross/src/$component" -o "$TEST_OUT/$name.$component_name.o" >> "$TEST_OUT/$name.build.log" 2>&1
    done
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections \
        "$TEST_OUT/$name.test.o" pc_port/src/battle_mips_adapter.c \
        "$TEST_OUT/$name.body.o" "$TEST_OUT/$name.compat.o" \
        "$TEST_OUT/$name.LIBGTE.C.o" "$TEST_OUT/$name.INLINE_C.C.o" \
        "$TEST_OUT/$name.PsyX_GTE.cpp.o" "$TEST_OUT/$name.half_float.cpp.o" \
        -lstdc++ -lm -Wl,--wrap=rand,--wrap=ReadGeomOffset,--wrap=ScaleMatrixL,\
--wrap=ScaleMatrix,--wrap=ApplyMatrixSV,--wrap=MulMatrix0 \
        -o "$TEST_OUT/$name.test" >> "$TEST_OUT/$name.build.log" 2>&1
}

failed=0
semantic_red=0

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

make_mutant() {
    local kind=$1
    local output="$TEST_OUT/animation_scripts.$kind.c"
    python3 - "$kind" src/slus_006.64/system/animation_scripts.c "$output" <<'PY'
from pathlib import Path
import sys

kind, source_name, output_name = sys.argv[1:]
text = Path(source_name).read_text()
mutations = {
    'narrow-input': (
        'u32 value = AnimationRead32(input + i * 4);',
        'u32 value = (u16)AnimationRead32(input + i * 4);'),
    'unsplit-input': (
        'quotient[i] = value >> 15;\n            remainder[i] = value & 0x7FFFu;',
        'quotient[i] = value;\n            remainder[i] = 0u;'),
    'swap-gte-stages': (
        'doCOP2(0x041E012);\n    for (i = 0; i < 3; ++i) high[i] = MFC2(25 + i);\n'
        '    for (i = 0; i < 3; ++i) MTC2(remainder[i], 9 + i);\n'
        '    doCOP2(0x049E012);',
        'doCOP2(0x049E012);\n    for (i = 0; i < 3; ++i) high[i] = MFC2(25 + i);\n'
        '    for (i = 0; i < 3; ++i) MTC2(remainder[i], 9 + i);\n'
        '    doCOP2(0x041E012);'),
    'saturated-output': (
        'result[i] = MFC2(25 + i) + (high[i] << 3);',
        'result[i] = (u32)(s16)MFC2(25 + i);'),
    'signed-range': (
        'u32 randomByte = (u32)rand() & 0xFFu;\n        u32 range = ((u8*)operands)[0];',
        'u32 randomByte = (u32)rand() & 0xFFu;\n        u32 range = (u32)(s32)(s8)((u8*)operands)[0];'),
    'reversed-trig': (
        'angles.vz = (s16)(delta * 16);',
        'angles.vz = (s16)(-delta * 16);'),
}
old, new = mutations[kind]
count = text.count(old)
if count != 1:
    raise SystemExit(f'{kind}: expected one mutation site, found {count}')
Path(output_name).write_text(text.replace(old, new, 1))
PY
    printf '%s\n' "$output"
}

run_negative_control() {
    local kind=$1
    local source
    source=$(make_mutant "$kind")
    local name="mutant_${kind//-/_}"
    if ! build_case "$name" O0 "$source"; then
        cat "$TEST_OUT/$name.build.log" >&2
        echo "SPRITE C4 MUTANT INFRASTRUCTURE FAILURE: $kind" >&2
        return 2
    fi
    local status=0
    "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        echo "SPRITE C4 MUTANT ACCEPTED: $kind" >&2
        return 1
    fi
    if ! rg -q 'SPRITE C4 FAIL case=' "$TEST_OUT/$name.log"; then
        cat "$TEST_OUT/$name.log" >&2
        echo "SPRITE C4 MUTANT UNEXPECTED FAILURE: $kind rc=$status" >&2
        return 1
    fi
    echo "SPRITE C4 MUTANT REJECTED: $kind"
}

for opt in O0 O2 UBSan; do
    if ! build_case "$opt" "$opt"; then
        cat "$TEST_OUT/$opt.build.log" >&2
        echo "SPRITE C4 INFRASTRUCTURE FAILURE: $opt" >&2
        exit 2
    fi
    status=0
    "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        cat "$TEST_OUT/$opt.log"
    elif rg -q 'sprite_animation_unimplemented.*"opcode":196' "$TEST_OUT/$opt.log"; then
        semantic_red=1
        echo "SPRITE C4 $opt SEMANTIC RED: current native dispatcher rejects opcode 0xC4" >&2
        rg 'sprite_animation_unimplemented|Assertion' "$TEST_OUT/$opt.log" >&2 || true
    else
        cat "$TEST_OUT/$opt.log" >&2
        echo "SPRITE C4 UNEXPECTED FAILURE: $opt rc=$status" >&2
        exit 1
    fi
done

if [ "$semantic_red" -ne 0 ]; then
    verify_source_pins
    echo "SPRITE C4 RED: retail C4 pins and runnable native-vs-retail fixture are ready; native implementation is still unsupported" >&2
    exit 1
fi

verify_source_pins
for mutant in narrow-input unsplit-input swap-gte-stages saturated-output signed-range reversed-trig; do
    run_negative_control "$mutant"
done
echo "SPRITE C4 GREEN: O0/O2/UBSan native-vs-retail finite census; $TEST_OUT"
