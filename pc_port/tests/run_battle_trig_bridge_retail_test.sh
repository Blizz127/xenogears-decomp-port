#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/battle_trig_bridge_retail_test.XXXXXXXX)
export TEST_OUT
echo "BATTLE TRIG BRIDGE artifacts: $TEST_OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
image = Path('disc/SLUS_006.64').read_bytes()
assert sha256(image).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
pins = {'SLUS_006.64': sha256(image).hexdigest()}
for name, address, size, expected in [
    ('trig_instructions', 0x8003f8b0, 0x38, '9687ca637b21311f891dd33fa756f8f1c31ff274ccf92923c699b087b44e4c92'),
    ('trig_table', 0x800523f0, 0x4000, 'b40c47b014ca8650c539760fd4c519cabce6f091628566ca0dd22318bb2c5b7a')]:
    offset = address - 0x8000f800
    actual = sha256(image[offset:offset + size]).hexdigest()
    assert actual == expected, (name, actual)
    pins[name] = {'address': hex(address), 'size': size, 'sha256': actual}
for name in ['pc_port/src/battle_mips_runtime.c', 'pc_port/src/battle_mips_adapter.c',
             'pc_port/extern/PsyCross/src/psx/LIBGTE.C',
             'pc_port/extern/PsyCross/src/gte/rcossin_tbl.h', 'pc_port/include_shim/psyq/libgte.h',
             'config/symbol_addrs.slus_006.64.txt', 'tools/scripts/gen_battle_bridge_map.py',
             'pc_port/tests/battle_trig_bridge_retail_test.c',
             'pc_port/tests/run_battle_trig_bridge_retail_test.sh']:
    data = Path(name).read_bytes()
    pins[name] = sha256(data).hexdigest()
    (out / Path(name).name).write_bytes(data)
pins['scope'] = 'Unaltered production runtime initialization and dispatch; real exported PsyCross trig functions; retail instructions/table as oracle; scalar v0 only.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --out "$TEST_OUT/battle_bridge_map.inc"
rg -q '0x8003f8b0u, 0u, 1, "rcos"' "$TEST_OUT/battle_bridge_map.inc"
rg -q '0x8003f8ccu, 0u, 1, "rsin"' "$TEST_OUT/battle_bridge_map.inc"

common=(-fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
failed=0
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h \
            -c "pc_port/extern/PsyCross/src/$source" -o "$TEST_OUT/$opt.$name.o"
    done
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$TEST_OUT" \
        -Wall -Wextra -Werror -c pc_port/tests/battle_trig_bridge_retail_test.c -o "$TEST_OUT/$opt.test.o"
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$TEST_OUT/$opt.cpu.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections \
        -Wl,--export-dynamic-symbol=rsin -Wl,--export-dynamic-symbol=rcos \
        -Wl,--section-start=.trig_psx_ram=0x10000020 -Wl,--section-start=.trig_scratch=0x10300040 \
        "$TEST_OUT/$opt.test.o" "$TEST_OUT/$opt.cpu.o" "$TEST_OUT/$opt.LIBGTE.C.o" \
        "$TEST_OUT/$opt.INLINE_C.C.o" "$TEST_OUT/$opt.PsyX_GTE.cpp.o" -ldl -o "$TEST_OUT/$opt.test"
    if "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1; then
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
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for name, expected in pins.items():
    if '/' in name:
        assert sha256(Path(name).read_bytes()).hexdigest() == expected, 'Source changed during test: ' + name
PY
if [ "$failed" != 0 ]; then
    echo "BATTLE TRIG BRIDGE RED: see $TEST_OUT (source snapshots, pins and all build-mode logs)" >&2
    exit 1
fi

# The positive test above asserts outcomes only. These source copies make each
# independently relevant regression concrete; production is never rewritten.
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
source = (out / 'battle_mips_runtime.c').read_text()
start_text = '    if (resolved->address == 0x8003f8b0u ||\n'
assert source.count(start_text) == 1
start = source.index(start_text)
end = source.index('\n    }\n', start) + len('\n    }\n')
block = source[start:end]
assert 'resolved->address == 0x8003f8ccu' in block
assert block.count('return 1;') == 1
binding_start_text = '        if (symbol->is_function && symbol->address == 0x8003f8b0u)\n'
assert source.count(binding_start_text) == 1
binding_start = source.index(binding_start_text)
binding_end = source.index('        if (host == NULL)', binding_start)
binding = source[binding_start:binding_end]
assert binding.count('"rsin"') == 1 and binding.count('"rcos"') == 1
def replace_once(old, new):
    assert block.count(old) == 1, old
    return block.replace(old, new)
wrong_binding = binding.replace('"rsin"', '"TEMP_TRIG_NAME"').replace('"rcos"', '"rsin"').replace('"TEMP_TRIG_NAME"', '"rcos"')
mutations = {
    'original-dispatch': ('', '        host = dlsym(RTLD_DEFAULT, symbol->name);\n'),
    'swapped-functions': (block, wrong_binding),
    'translated-scalar': (replace_once('cpu->gpr[4] & 0xfffu', 'translate_argument(runtime, cpu->gpr[4]) & 0xfffu'), binding),
    'missing-mask': (replace_once('(cpu->gpr[4] & 0xfffu)', 'cpu->gpr[4]'), binding),
}
manifest = {'production_sha256': sha256(source.encode()).hexdigest(), 'mutants': {}}
for name, (mutant, mutant_binding) in mutations.items():
    result = source[:start] + mutant + source[end:]
    assert result.count(binding) == 1
    result = result.replace(binding, mutant_binding)
    assert result != source
    (out / (name + '.c')).write_text(result)
    manifest['mutants'][name] = {'sha256': sha256(result.encode()).hexdigest(),
                                'build': 'UBSan' if name == 'missing-mask' else 'O2'}
(out / 'negative-controls.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY

for mutant in original-dispatch swapped-functions translated-scalar missing-mask; do
    opt=O2
    flags=(-O2)
    if [ "$mutant" = missing-mask ]; then
        opt=UBSan
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$TEST_OUT" \
        "-DBATTLE_RUNTIME_SOURCE=\"$PWD/$TEST_OUT/$mutant.c\"" \
        -Wall -Wextra -Werror -c pc_port/tests/battle_trig_bridge_retail_test.c -o "$TEST_OUT/$mutant.test.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections \
        -Wl,--export-dynamic-symbol=rsin -Wl,--export-dynamic-symbol=rcos \
        -Wl,--section-start=.trig_psx_ram=0x10000020 -Wl,--section-start=.trig_scratch=0x10300040 \
        "$TEST_OUT/$mutant.test.o" "$TEST_OUT/$opt.cpu.o" "$TEST_OUT/$opt.LIBGTE.C.o" \
        "$TEST_OUT/$opt.INLINE_C.C.o" "$TEST_OUT/$opt.PsyX_GTE.cpp.o" -ldl -o "$TEST_OUT/$mutant.test"
    if "$TEST_OUT/$mutant.test" > "$TEST_OUT/$mutant.log" 2>&1; then
        echo "BATTLE TRIG BRIDGE FAIL mutant survived: $mutant" >&2
        exit 1
    fi
    rg -q 'BATTLE TRIG TABLE PASS 8192 comparisons' "$TEST_OUT/$mutant.log"
    if [ "$mutant" = missing-mask ]; then
        rg -q 'negation of -2147483648 cannot be represented|BATTLE TRIG BRIDGE FAIL range=' "$TEST_OUT/$mutant.log"
    else
        rg -q 'BATTLE TRIG BRIDGE FAIL range=' "$TEST_OUT/$mutant.log"
    fi
    echo "BATTLE TRIG BRIDGE negative control PASS: $mutant ($opt)"
done
