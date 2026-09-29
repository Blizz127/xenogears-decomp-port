#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_f2_retail_test.XXXXXXXX)
export TEST_OUT
echo "SPRITE F2 artifacts: $TEST_OUT"

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
    ('handler', 0x80020fc8, 0x800210f0, '609117e66e017921ac77c43d1215aef4ee7848a403d4109280e5523a3df5cbe8'),
    ('clamp', 0x80021ad8, 0x80021b04, '20ee2dd52999178b6e1e6136ddcbe09ca0928b67c3b92c3a7d7d050121ccaf9a'),
    ('leaf', 0x8001f6b0, 0x8001f750, 'cb7dff828c141c1959ce71eba1492665e5bdf561cc0f7a9859ed08ac6efe2884')]:
    actual = sha256(image[start - 0x8000f800:end - 0x8000f800]).hexdigest()
    assert actual == expected, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', image, 0x800183d8 + (0xf2 - 0x8a) * 4 - 0x8000f800)[0] == 0x80020fc8
for path in ['src/slus_006.64/system/animation_scripts.c', 'src/slus_006.64/system/rendering.c',
             'pc_port/src/battle_mips_adapter.c', 'pc_port/src/psyq_compat.c', 'pc_port/include_shim/psx_memory.h',
             'pc_port/src/port_compat.h', 'pc_port/extern/PsyCross/src/psx/LIBGTE.C',
             'pc_port/extern/PsyCross/src/psx/INLINE_C.C', 'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp',
             'pc_port/extern/PsyCross/src/gte/half_float.cpp', 'pc_port/tests/sprite_dispatch_f2_retail_test.c',
             'pc_port/tests/run_sprite_dispatch_f2_retail_test.sh']:
    data = Path(path).read_bytes(); pins[path] = sha256(data).hexdigest(); (out / Path(path).name).write_bytes(data)
source = Path('src/slus_006.64/system/rendering.c').read_text()
start = source.index('void func_8001F6B0(void* pSpriteData)'); end = source.index('\n}\n', start) + 3
leaf = source[start:end]
assert leaf.count('void func_8001F6B0(') == 1
(out / 'f2_leaf.c').write_text('#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;\n' + leaf.replace('void func_8001F6B0(', 'void f2_leaf_impl(', 1))
pins['leaf_source_sha256'] = sha256(leaf.encode()).hexdigest()
pins['scope'] = 'One actual retail F2 handler with RGB clamp calls, mode-1 leaf, six-argument 800B2AEC call observation (leaf unverified), backed pointers and signed operand deltas; no full B2AEC or rendered-output claim.'
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
    gcc -std=gnu17 -fpermissive "${common[@]}" -Ipc_port/include_shim -Iinclude -Ipc_port/src -c "$TEST_OUT/f2_leaf.c" -o "$TEST_OUT/$name.leaf.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -Ipc_port/src -c pc_port/tests/sprite_dispatch_f2_retail_test.c -o "$TEST_OUT/$name.test.o" >> "$TEST_OUT/$name.build.log" 2>&1
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
text = Path(src).read_text(); start = text.index('    case 0xF2:'); end = text.index('    case ', start + 1); body = text[start:end]
mutations = {
    'unsigned-delta': ('(s8)source[0]', 'source[0]'),
    'wrong-mode2-gate': ('(flags & 3u) == 2u', '(flags & 3u) == 1u'),
    'wrong-type-gate': ('== 15u', '!= 15u'),
    'missing-leaf': ('func_8001F6B0(p);', '(void)0;'),
    'wrong-reload': ('AnimationRead32(currentBase + 0x2C)', 'AnimationRead32(base + 0x2C)'),
}
old, new = mutations[kind]
if kind == 'unsigned-delta':
    for channel in range(3):
        old_channel = f'(s8)source[{channel}]'
        assert body.count(old_channel) == 2, (kind, channel, body.count(old_channel))
        body = body.replace(old_channel, f'source[{channel}]')
else:
    assert body.count(old) == 1, (kind, body.count(old))
    body = body.replace(old, new)
Path(dst).write_text(text[:start] + body + text[end:])
PY
    printf '%s\n' "$output"
}

run_control() {
    local kind=$1 source=$2; shift 2
    build_case "control_$kind" O2 "$source"
    local status=0
    env SPRITE_F2_QUICK=1 SPRITE_F2_VECTOR="${1:-2}" SPRITE_F2_MODE="${2:-1}" \
        SPRITE_F2_TYPE="${3:-15}" SPRITE_F2_BIT1="${4:-0}" SPRITE_F2_HEADER="${5:-1}" \
        SPRITE_F2_ALIAS="${6:-0}" "$TEST_OUT/control_$kind.test" > "$TEST_OUT/control_$kind.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ] || ! rg -q '^SPRITE F2 FAIL ' "$TEST_OUT/control_$kind.log"; then
        cat "$TEST_OUT/control_$kind.log" >&2; echo "SPRITE F2 CONTROL SURVIVED: $kind" >&2; exit 1
    fi
    echo "SPRITE F2 control rejected $kind: $(rg -m1 '^SPRITE F2 FAIL ' "$TEST_OUT/control_$kind.log")"
}

if [ "${SPRITE_F2_RUN_GREEN:-1}" = 1 ]; then
    for opt in O0 O2 UBSan; do
        build_case "$opt" "$opt"
        "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1
        cat "$TEST_OUT/$opt.log"
    done
    for kind in unsigned-delta wrong-mode2-gate wrong-type-gate missing-leaf wrong-reload; do
        mutant=$(make_mutant "$kind")
        case "$kind" in
            wrong-mode2-gate) run_control "$kind" "$mutant" 2 2 15 0 1 0 ;;
            wrong-type-gate) run_control "$kind" "$mutant" 2 1 0 0 1 0 ;;
            wrong-reload) run_control "$kind" "$mutant" 9 2 15 0 1 3 ;;
            *) run_control "$kind" "$mutant" 2 1 15 0 1 0 ;;
        esac
    done
    python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
pins=json.loads((Path(os.environ['TEST_OUT'])/'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path: assert sha256(Path(path).read_bytes()).hexdigest()==expected, 'Source changed during test: '+path
PY
    echo "SPRITE F2 GREEN: O0/O2/UBSan finite census plus 5 controls; $TEST_OUT"
    exit 0
fi

for opt in O0 O2 UBSan; do
    build_case "$opt" "$opt"
    status=0
    "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ] || ! rg -q 'sprite_animation_unimplemented.*"opcode":242' "$TEST_OUT/$opt.log"; then
        cat "$TEST_OUT/$opt.log" >&2
        echo "SPRITE F2 UNEXPECTED RESULT $opt rc=$status" >&2
        exit 1
    fi
    echo "SPRITE F2 $opt SEMANTIC RED: native dispatcher rejects opcode 0xF2"
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
echo "SPRITE F2 RED: one backed retail clamp/leaf/B2 call-observation fixture is runnable; native F2 remains unsupported"
