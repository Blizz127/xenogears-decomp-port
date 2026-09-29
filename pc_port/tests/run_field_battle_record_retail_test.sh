#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/field_battle_record_retail_test.XXXXXXXX)
export TEST_OUT
echo "FIELD BATTLE RECORD artifacts: $TEST_OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os, re, struct
out = Path(os.environ['TEST_OUT'])
pins = {}
for name, path, address, size, base, expected in [
    ('field_call', 'disc/field.bin', 0x8007104c, 0x24, 0x8006faf0, 'ffb77b50eee3aa64a3d24a48eeaca360a5be34654dfd888ebc934519d9fe098b'),
    ('field_wrapper', 'disc/field.bin', 0x8007008c, 0x24, 0x8006faf0, '043363a6b87236971c393ce02b1fa3611823521f624c52a2fdb21c79d685b71c'),
    ('lzss', 'disc/SLUS_006.64', 0x80032eb4, 0xa0, 0x8000f800, '603f626038f3ee725fe0b5a79988343ff0aca07e6c2a72a95791d3d565f926ed')]:
    data = Path(path).read_bytes()[address - base:address - base + size]
    assert sha256(data).hexdigest() == expected, name
    pins[name] = {'address': hex(address), 'size': size, 'sha256': expected}
with open('disc/disc1.bin', 'rb') as disc:
    data = bytearray()
    for offset in range(0, 123808, 2048):
        disc.seek((121096 + offset // 2048) * 2352 + 24)
        data.extend(disc.read(min(2048, 123808 - offset)))
assert sha256(data).hexdigest() == 'd81670fa78aeed510852153349cd8d4e4b5a6bc6469110e41fb6274aa4e4261f'
assert struct.unpack_from('<I', data, 0x124)[0] == 528
source_offset = struct.unpack_from('<I', data, 0x148)[0]
assert source_offset == 121448 and struct.unpack_from('<I', data, source_offset)[0] == 530
pins['map2'] = {'sector': 121096, 'size': len(data), 'sha256': sha256(data).hexdigest(),
                'metadata_size': 528, 'stream_size': 530, 'compressed_offset': source_offset}

field = Path('src/field/main/misc3.c').read_text()
overrides = Path('pc_port/src/game_overrides.c').read_text()
def function(source, signature):
    assert source.count(signature) == 1
    start = source.index(signature)
    end = source.index('\n}', start) + 2
    return source[start:end]
wrapper = function(field, 'void FieldLZSSDecompress(')
decoder = function(overrides, 'void* LZSSDecompress(')
field_start = field.index('void FieldLoad(void)')
calls = [m for m in re.finditer(r'FieldLZSSDecompress\([\s\S]*?\);', field[field_start:])
         if '0x148' in m.group()]
assert len(calls) == 1 and 'D_800658DC' in calls[0].group()
call_start = field_start + calls[0].start()
call_start = field.rfind('\n', 0, call_start) + 1
call = field[call_start:field_start + calls[0].end()]
assert call.count('FieldLZSSDecompress(') == 1
def extracted(call_body):
    return decoder + '\n\n' + wrapper + '\n\nstatic void RunProductionFieldBattleCall(void)\n{\n' + call_body + '\n}\n'
(out / 'production.c').write_text(extracted(call))
(out / 'production-call.c').write_text(call + '\n')
pins['field_source_sha256'] = sha256(field.encode()).hexdigest()
pins['overrides_source_sha256'] = sha256(overrides.encode()).hexdigest()
pins['extracted_source_sha256'] = sha256(extracted(call).encode()).hexdigest()
for path in ['pc_port/tests/field_battle_record_retail_test.c',
             'pc_port/tests/run_field_battle_record_retail_test.sh',
             'pc_port/src/battle_mips_adapter.c']:
    content = Path(path).read_bytes()
    pins[path] = sha256(content).hexdigest()
    (out / Path(path).name).write_bytes(content)
pins['scope'] = 'Verbatim production FieldLoad section call plus wrapper and decoder; retail instruction oracle; guarded contiguous destination fixture, not host symbol ownership.'
(out / 'misc3.c').write_text(field)
(out / 'game_overrides.c').write_text(overrides)
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -Ipc_port/src -Wall -Wextra -Werror -Wno-unused-parameter)
failed=0
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" \
        "-DFIELD_BATTLE_SOURCE=\"$PWD/$TEST_OUT/production.c\"" \
        -c pc_port/tests/field_battle_record_retail_test.c -o "$TEST_OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$TEST_OUT/$opt.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$TEST_OUT/$opt.test.o" "$TEST_OUT/$opt.cpu.o" -o "$TEST_OUT/$opt.test"
    if "$TEST_OUT/$opt.test" "$TEST_OUT/$opt.oracle.bin" > "$TEST_OUT/$opt.log" 2>&1; then
        cat "$TEST_OUT/$opt.log"
    else
        cat "$TEST_OUT/$opt.log" >&2
        failed=1
    fi
done
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
pins = json.loads((out / 'provenance.json').read_text())
assert sha256(Path('src/field/main/misc3.c').read_bytes()).hexdigest() == pins['field_source_sha256'], 'Field source changed during test'
assert sha256(Path('pc_port/src/game_overrides.c').read_bytes()).hexdigest() == pins['overrides_source_sha256'], 'Decoder source changed during test'
for opt in ['O0', 'O2', 'UBSan']:
    assert sha256((out / (opt + '.oracle.bin')).read_bytes()).hexdigest() == '727bad94d380db9065d190fb7c28cc422549030dcaab9edee894b186fbbf85e9'
PY
if [ "$failed" != 0 ]; then
    echo "FIELD BATTLE RECORD RED: see $TEST_OUT (source snapshots, call, pins and full retail output)" >&2
    exit 1
fi

python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
source = (out / 'production.c').read_text()
call = (out / 'production-call.c').read_text().removesuffix('\n')
assert source.count(call) == 1 and call.count('D_800658DC') == 1
assert 'D_800658DC);' in call, 'Negative controls require the unshifted production destination'
mutations = {
    'old-destination-shift': call.replace('D_800658DC', 'D_800658DC + 0x10'),
    'one-byte-shift': call.replace('D_800658DC', 'D_800658DC + 1'),
    # Corrupt a non-flag byte of selector1, then a byte past map metadata's
    # 528-byte size. Both must fail a complete-output comparison independently.
    'record-corruption': call + '\n    D_800658DC[32 + 18] ^= 1;',
    'tail-corruption': call + '\n    D_800658DC[528] ^= 1;',
}
manifest = {}
for name, changed in mutations.items():
    mutated = source.replace(call, changed)
    assert mutated != source
    (out / (name + '.c')).write_text(mutated)
    manifest[name] = sha256(mutated.encode()).hexdigest()
(out / 'negative-controls.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY
for mutant in old-destination-shift one-byte-shift record-corruption tail-corruption; do
    gcc "${common[@]}" -O2 \
        "-DFIELD_BATTLE_SOURCE=\"$PWD/$TEST_OUT/$mutant.c\"" \
        -c pc_port/tests/field_battle_record_retail_test.c -o "$TEST_OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$TEST_OUT/$mutant.o" "$TEST_OUT/O2.cpu.o" -o "$TEST_OUT/$mutant.test"
    if "$TEST_OUT/$mutant.test" "$TEST_OUT/$mutant.oracle.bin" > "$TEST_OUT/$mutant.log" 2>&1; then
        echo "FIELD BATTLE RECORD FAIL mutant survived: $mutant" >&2
        exit 1
    fi
    rg -q 'FIELD BATTLE RECORD FAIL fill=' "$TEST_OUT/$mutant.log"
    echo "FIELD BATTLE RECORD negative control PASS: $mutant"
done
