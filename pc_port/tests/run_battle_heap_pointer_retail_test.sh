#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/battle_heap_pointer_retail_test.XXXXXXXX)
export TEST_OUT
echo "BATTLE HEAP POINTER artifacts: $TEST_OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
image = Path('disc/battle.bin').read_bytes()
assert sha256(image).hexdigest() == '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'
pins = {'battle_image_sha256': sha256(image).hexdigest()}
for name, address, size, expected in [
    ('reservation_fragment', 0x80070e48, 0x40, '44982c631b8d92bb830a3120ac7be992f4b49b8b5f338253bedf588781464e65'),
    ('allocation_helper', 0x8008abb8, 0x48, '0ccc7960a341132dd239cb1d897031506b35f479855f21bf8a75e994199369e5')]:
    offset = address - 0x8006faf0
    assert sha256(image[offset:offset + size]).hexdigest() == expected, name
    pins[name] = {'address': hex(address), 'size': size, 'sha256': expected}
for name in ['pc_port/src/battle_mips_runtime.c', 'pc_port/src/battle_mips_adapter.c',
             'pc_port/src/psx_memory.h', 'config/symbol_addrs.slus_006.64.txt',
             'tools/scripts/gen_battle_bridge_map.py', 'pc_port/tests/battle_heap_pointer_retail_test.c',
             'pc_port/tests/run_battle_heap_pointer_retail_test.sh']:
    data = Path(name).read_bytes()
    pins[name] = sha256(data).hexdigest()
    (out / Path(name).name).write_bytes(data)
pins['scope'] = 'Actual runtime initialization/dispatch and retail battle reservation instructions; native allocator returns host RAM pointers, oracle allocator returns same guest locations; no allocator-policy or rendering proof.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY
bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --out "$TEST_OUT/battle_bridge_map.inc"

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
        -Wall -Wextra -Werror -c pc_port/tests/battle_heap_pointer_retail_test.c -o "$TEST_OUT/$opt.test.o"
    gcc -std=gnu17 "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$TEST_OUT/$opt.cpu.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections \
        -Wl,--export-dynamic-symbol=HeapAlloc -Wl,--export-dynamic-symbol=HeapChangeCurrentUser -Wl,--export-dynamic-symbol=HeapFree \
        -Wl,--section-start=.heap_test_ram=0x0060fe20 -Wl,--section-start=.bss=0x02000000 \
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
    echo "BATTLE HEAP POINTER RED: see $TEST_OUT (source snapshots, pins and all build-mode logs)" >&2
    exit 1
fi

python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
out = Path(os.environ['TEST_OUT'])
source = (out / 'battle_mips_runtime.c').read_text()
start_text = '    if (resolved->address == 0x80031bdcu) {\n'
assert source.count(start_text) == 1
start = source.index(start_text)
end = source.index('\n    }\n', start) + len('\n    }\n')
block = source[start:end]
assert block.count('result = PsxMemory_GuestAddr((void *)result);') == 1
assert source.count('translate_argument(runtime, raw)') == 2
mutations = {
    'raw-host-return': source[:start] + source[end:],
    'normalize-scalar-return': source.replace(start_text, '    {\n'),
    'missing-argument-translation': source.replace('translate_argument(runtime, raw)', '(uintptr_t)raw'),
}
manifest = {'production_sha256': sha256(source.encode()).hexdigest(), 'mutants': {}}
for name, mutant in mutations.items():
    assert mutant != source
    (out / (name + '.c')).write_text(mutant)
    manifest['mutants'][name] = sha256(mutant.encode()).hexdigest()
(out / 'negative-controls.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY
for mutant in raw-host-return normalize-scalar-return missing-argument-translation; do
    gcc -std=gnu17 "${common[@]}" -O2 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$TEST_OUT" \
        "-DBATTLE_RUNTIME_SOURCE=\"$PWD/$TEST_OUT/$mutant.c\"" \
        -Wall -Wextra -Werror -c pc_port/tests/battle_heap_pointer_retail_test.c -o "$TEST_OUT/$mutant.o"
    clang++ -no-pie -Wl,--gc-sections \
        -Wl,--export-dynamic-symbol=HeapAlloc -Wl,--export-dynamic-symbol=HeapChangeCurrentUser -Wl,--export-dynamic-symbol=HeapFree \
        -Wl,--section-start=.heap_test_ram=0x0060fe20 -Wl,--section-start=.bss=0x02000000 \
        "$TEST_OUT/$mutant.o" "$TEST_OUT/O2.cpu.o" "$TEST_OUT/O2.LIBGTE.C.o" \
        "$TEST_OUT/O2.INLINE_C.C.o" "$TEST_OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$TEST_OUT/$mutant.test"
    if "$TEST_OUT/$mutant.test" > "$TEST_OUT/$mutant.log" 2>&1; then
        echo "BATTLE HEAP POINTER FAIL mutant survived: $mutant" >&2
        exit 1
    fi
    if [ "$mutant" = raw-host-return ]; then
        rg -q 'BATTLE HEAP POINTER FAIL reservation' "$TEST_OUT/$mutant.log"
    else
        rg -q 'BATTLE HEAP POINTER FAIL roundtrip' "$TEST_OUT/$mutant.log"
    fi
    echo "BATTLE HEAP POINTER negative control PASS: $mutant"
done
