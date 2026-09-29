#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/field_gear_model_list_retail_test.XXXXXXXX)
export TEST_OUT
echo "GEAR MODEL LIST artifacts: $TEST_OUT"
python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os, re

out = Path(os.environ['TEST_OUT'])
pins = {}
with open('disc/disc1.bin', 'rb') as disc:
    for name, sector, size, expected in [
        ('overlay', 231361, 25 * 2048, '14395a9f54c124fed016f07906cc882b2a9256d5ade2d10eef99e10fe9366523'),
        ('model', 232272, 1256, '9fe50ec9a6b732c50f4ff2e41baa6e76a297fd9eff3d9814c3087e4c11cf7a92'),
        ('texture', 232273, 44984, 'f1396a01be36f92a682321372e0f4e558f78e2f30af4811c9a50cef55c6bc5cd')]:
        data = bytearray()
        for offset in range(0, size, 2048):
            disc.seek((sector + offset // 2048) * 2352 + 24)
            data.extend(disc.read(min(2048, size - offset)))
        actual = sha256(data).hexdigest()
        assert actual == expected, (name, actual)
        pins[name] = {'sector': sector, 'size': size, 'sha256': actual}
        if name == 'overlay':
            assert data[0xb5a8:0xb5ac] == bytes.fromhex('0c00908e') # lw s0,12(s4)
            assert sha256(data[0xb42c:0xb7d8]).hexdigest() == '465d5701dffb89137ea6fcc129a4ad284eab40119fb37c5c4df4402c227510c5'
            assert sha256(data[0x2d0:0x5c0]).hexdigest() == 'd10a984d37284d898fcea03ff7360ef9d0cb0a0e0e41046775aa7020c1681e6a'

source = Path('pc_port/src/field_object_overlay.c').read_text()
start = source.index('void func_801E742C(')
finish = source.index('\n}\n', start) + 3
call = source.index('    nodes = OvlyBuildNodes(', start, finish)
end = source.index(';', call, finish) + 1
assert source[end:finish].startswith('\n    *(u32*)(obj + 4) =')
prefix = source[start:end]
assert prefix.count('OvlyBuildNodes(') == 1
assert 'OvlyClip' not in prefix
declaration = '''static u8* CaptureNodes(OvlyPtrTab*, u16*, s32, s32, s16, s16, s16, s16);
#define OvlyBuildNodes CaptureNodes
'''
def extract(body):
    return source[:start] + declaration + body + '\n}\n#undef OvlyBuildNodes\n' + source[finish:]

(out / 'production-source.c').write_text(source)
(out / 'production-prefix.c').write_text(prefix + '\n')
(out / 'loader.c').write_text(extract(prefix))
pins['source_sha256'] = sha256(source.encode()).hexdigest()
pins['prefix_sha256'] = sha256(prefix.encode()).hexdigest()
pins['seam'] = 'Verbatim production 742C prefix through node call; callee capture wrapper; oracle enters 758C with relocated archives; clip suffix excluded.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')

# Reintroduce wrong archive/entry choices without changing the test oracle.
assignment = re.compile(r'^\s+list = [^;]+;$', re.M)
assert len(assignment.findall(prefix)) == 1
for name, expression in {
    'old-model-plus8': '(u16*)(uintptr_t)*(u32*)(modelArc + 8)',
    'texture-plus8': '(u16*)(uintptr_t)*(u32*)(tex + 8)',
    'model-plus4': '(u16*)(uintptr_t)*(u32*)(modelArc + 4)',
    'texture-base': '(u16*)tex',
    'model-plusC': '(u16*)(uintptr_t)*(u32*)(modelArc + 0xC)',
    'missing-list': 'NULL',
    'late-texture-plusC': '(u16*)(uintptr_t)*(u32*)(tex + 0xC)',
}.items():
    mutated, count = assignment.subn('\n        list = ' + expression + ';', prefix)
    assert count == 1
    (out / (name + '.c')).write_text(extract(mutated))
PY

common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
failed=0
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$PWD/$TEST_OUT/loader.c\"" \
        -c pc_port/tests/field_gear_model_list_retail_test.c -o "$TEST_OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$TEST_OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections \
        "$TEST_OUT/$opt.test.o" "$TEST_OUT/$opt.cpu.o" -o "$TEST_OUT/$opt.test"
    if "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1; then
        cat "$TEST_OUT/$opt.log"
    else
        cat "$TEST_OUT/$opt.log" >&2
        failed=1
    fi
done
if [ "$failed" != 0 ]; then
    echo "GEAR MODEL LIST RED: see $TEST_OUT (source, prefix, pins and all build-mode logs)" >&2
    exit 1
fi

for mutant in old-model-plus8 texture-plus8 model-plus4 texture-base model-plusC missing-list late-texture-plusC; do
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DOBJECT_OVERLAY_SOURCE=\"$PWD/$TEST_OUT/$mutant.c\"" \
        -c pc_port/tests/field_gear_model_list_retail_test.c -o "$TEST_OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$TEST_OUT/$mutant.o" "$TEST_OUT/O2.cpu.o" -o "$TEST_OUT/$mutant.test"
    if "$TEST_OUT/$mutant.test" > "$TEST_OUT/$mutant.log" 2>&1; then
        echo "GEAR MODEL LIST FAIL mutant survived: $mutant" >&2
        exit 1
    fi
    rg -q 'GEAR MODEL LIST FAIL variant=' "$TEST_OUT/$mutant.log"
done
echo "GEAR MODEL LIST negative controls PASS: old-model-plus8 texture-plus8 model-plus4 texture-base model-plusC missing-list late-texture-plusC"
