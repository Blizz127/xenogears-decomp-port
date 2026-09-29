#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/battle_primitive_colors_retail_test.XXXXXXXX)
export TEST_OUT
echo "B2AEC artifacts: $TEST_OUT"

python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
out = Path(os.environ['TEST_OUT'])
battle = Path('disc/battle.bin').read_bytes()
assert sha256(battle).hexdigest() == '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'
pins = {'disc/battle.bin': sha256(battle).hexdigest()}
start, end, expected = 0x800b2aec, 0x800b3348, 'a16aa67b48a5c644f8abbe4d2ae75cc068122e2eccfea67bc21175408b91d03c'
actual = sha256(battle[start-0x8006faf0:end-0x8006faf0]).hexdigest()
assert actual == expected
pins['b2aec'] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
slus = Path('disc/SLUS_006.64').read_bytes()
pins['disc/SLUS_006.64'] = sha256(slus).hexdigest()
start, end, expected = 0x80021ad8, 0x80021b04, '20ee2dd52999178b6e1e6136ddcbe09ca0928b67c3b92c3a7d7d050121ccaf9a'
actual = sha256(slus[start-0x8000f800:end-0x8000f800]).hexdigest()
assert actual == expected
pins['clamp'] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
source = Path('src/slus_006.64/system/animation_scripts.c').read_text()
start = source.index('s32 func_80021AD8(s32 color, s32 value) {')
end = source.index('\n}\n', start) + 3
clamp = source[start:end]
clamp_impl = clamp.replace('s32 func_80021AD8(', 'int32_t func_80021AD8(', 1)
(out / 'clamp.c').write_text('#include <stdint.h>\ntypedef int32_t s32;\n' + clamp_impl)
pins['clamp_source_sha256'] = sha256(clamp.encode()).hexdigest()
for path in ['pc_port/src/battle_mips_adapter.c', 'pc_port/src/battle_mips_adapter.h',
             'pc_port/tests/battle_primitive_colors_retail_test.c',
             'pc_port/tests/run_battle_primitive_colors_retail_test.sh']:
    data = Path(path).read_bytes(); pins[path] = sha256(data).hexdigest()
    (out / Path(path).name).write_bytes(data)
if Path('src/battle/primitive_colors.inc').exists():
    data = Path('src/battle/primitive_colors.inc').read_bytes()
    pins['src/battle/primitive_colors.inc'] = sha256(data).hexdigest()
pins['scope'] = ('Actual battle.bin B2AEC and SLUS clamp instructions versus native leaf and clamp; all 16 keys, '
                 'finite color/delta edges, zero/multiple records, strides, overlap and guards; '
                 'native scratch-empty RED when primitive_colors.inc is absent; no exhaustive universe claim.')
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

if [ -f src/battle/primitive_colors.inc ]; then
    NATIVE_SOURCE=src/battle/primitive_colors.inc
    NATIVE_MODE=production
else
    NATIVE_SOURCE="$TEST_OUT/primitive_colors_empty.c"
    NATIVE_MODE=scratch-empty
    cat > "$NATIVE_SOURCE" <<'C'
#include <stdint.h>
typedef uint8_t u8; typedef uint32_t u32; typedef int32_t s32;
void func_800B2AEC(void *header, void *buffer0, void *buffer1,
                   s32 red, s32 green, s32 blue)
{
    (void)header; (void)buffer0; (void)buffer1;
    (void)red; (void)green; (void)blue;
}
C
fi
export NATIVE_SOURCE NATIVE_MODE

build_case() {
    local name=$1 opt=$2
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/src -Iinclude -Ipc_port/include_shim -include assert.h -include stdint.h -include include/types.h -D_LANGUAGE_C -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    gcc -std=gnu17 "${common[@]}" -x c -c "$NATIVE_SOURCE" -o "$TEST_OUT/$name.native.o" > "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -c "$TEST_OUT/clamp.c" -o "$TEST_OUT/$name.clamp.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -c pc_port/tests/battle_primitive_colors_retail_test.c -o "$TEST_OUT/$name.test.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 "${common[@]}" -c pc_port/src/battle_mips_adapter.c -o "$TEST_OUT/$name.adapter.o" >> "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections \
        "$TEST_OUT/$name.test.o" "$TEST_OUT/$name.adapter.o" "$TEST_OUT/$name.native.o" "$TEST_OUT/$name.clamp.o" \
        -o "$TEST_OUT/$name.test" >> "$TEST_OUT/$name.build.log" 2>&1
}

make_mutant() {
    local name=$1 old=$2 new=$3
    python3 - "$NATIVE_SOURCE" "$TEST_OUT/$name.native.c" "$old" "$new" <<'PY'
from pathlib import Path
import sys
source = Path(sys.argv[1]).read_text()
old, new = sys.argv[3], sys.argv[4]
count = source.count(old)
if count != 1:
    raise SystemExit(f'mutation {old!r} matched {count} times')
Path(sys.argv[2]).write_text(source.replace(old, new, 1))
PY
}

run_mutant() {
    local name=$1 old=$2 new=$3
    local saved_source=$NATIVE_SOURCE
    make_mutant "$name" "$old" "$new"
    NATIVE_SOURCE="$TEST_OUT/$name.native.c"
    if ! build_case "$name" O2; then
        cat "$TEST_OUT/$name.build.log" >&2
        echo "B2AEC CONTROL INFRASTRUCTURE FAILURE: $name" >&2
        exit 2
    fi
    local status=0
    "$TEST_OUT/$name.test" > "$TEST_OUT/$name.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        echo "B2AEC CONTROL FAILED TO REJECT: $name" >&2
        failed=1
    elif [ "$status" -eq 1 ] && grep -q '^B2AEC FAIL case=' "$TEST_OUT/$name.log"; then
        echo "B2AEC CONTROL PASS: $name rejected";
    elif [ "$name" = guest_pointer_pass_through ] && [ "$status" -eq 139 ]; then
        echo "B2AEC CONTROL PASS: $name rejected (original invalid guest dereference)";
    else
        cat "$TEST_OUT/$name.log" >&2
        echo "B2AEC CONTROL UNEXPECTED FAILURE: $name rc=$status" >&2
        exit 2
    fi
    NATIVE_SOURCE=$saved_source
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

failed=0
for opt in O0 O2 UBSan; do
    if ! build_case "$opt" "$opt"; then
        cat "$TEST_OUT/$opt.build.log" >&2
        echo "B2AEC INFRASTRUCTURE FAILURE: $opt" >&2
        exit 2
    fi
    status=0
    "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        cat "$TEST_OUT/$opt.log"
    elif grep -q '^B2AEC FAIL case=' "$TEST_OUT/$opt.log"; then
        cat "$TEST_OUT/$opt.log" >&2
        failed=1
    else
        cat "$TEST_OUT/$opt.log" >&2
        echo "B2AEC UNEXPECTED FAILURE: $opt rc=$status" >&2
        exit 1
    fi
done

if [ "$NATIVE_MODE" = production ]; then
    run_mutant guest_pointer_pass_through \
        'return PSX_ADDR(address);' \
        'return pointer;'
    run_mutant key_invert \
        '(((descriptor[2] ^ 1u) & 1u) << 8)' \
        '(((descriptor[2] ^ 0u) & 1u) << 8)'
    run_mutant packet_shape \
        'sourceOffset = 0x04; triples = 1; break;' \
        'sourceOffset = 0x08; triples = 1; break;'
    run_mutant constant_128 \
        'constantColor ? 0x80' \
        'constantColor ? 0x00'
    run_mutant signed_delta \
        'func_80021AD8(color, deltas[channel])' \
        'func_80021AD8(color, (deltas[channel] < 0 ? 0 : deltas[channel]))'
    run_mutant buffer1_copy \
        'out1[offset] = out0[offset];' \
        'out1[offset] = 0;'
    run_mutant packet_stride \
        'packetBytes = ((u32)descriptor[0] + 1u) * 4u;' \
        'packetBytes = ((u32)descriptor[0] + 2u) * 4u;'
    run_mutant descriptor_stride \
        'descriptorBytes = ((u32)descriptor[1] + 1u) * 4u;' \
        'descriptorBytes = ((u32)descriptor[1] + 2u) * 4u;'
fi

verify_source_pins
if [ "$NATIVE_MODE" = scratch-empty ]; then
    echo "B2AEC RED: scratch empty native implementation only; production primitive_colors.inc is absent" >&2
    exit 1
fi
if [ "$failed" -ne 0 ]; then
    echo "B2AEC RED: native/retail mismatch" >&2
    exit 1
fi
echo "B2AEC GREEN: O0/O2/UBSan finite native-vs-retail census; $TEST_OUT"
