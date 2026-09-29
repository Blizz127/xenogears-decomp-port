#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."

SOURCE=${BATTLE_WINDOW_RUNTIME_SOURCE:-"$PWD/pc_port/src/battle_mips_runtime.c"}
case "$SOURCE" in
    /*) ;;
    *) SOURCE="$PWD/$SOURCE" ;;
esac
test -f "$SOURCE"
OUT=${WINDOW_ABI_TEST_OUT:-"$(mktemp -d /tmp/xeno-window-abi-test-20260906.XXXXXXXX)"}
mkdir -p "$OUT"
echo "BATTLE WINDOW ABI artifacts: $OUT"
echo "BATTLE WINDOW ABI source: $SOURCE"

python3 - "$OUT" "$SOURCE" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys

out = Path(sys.argv[1])
source = Path(sys.argv[2]).resolve()
exe = Path('disc/SLUS_006.64').read_bytes()
assert sha256(exe).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
base = 0x8000f800
start, end = 0x80032f54, 0x80032fa8
slice_hash = sha256(exe[start - base:end - base]).hexdigest()
assert slice_hash == 'c8adb46583be9b0d5327bc326b3100f130818ccec5f4b7a158740a25d788a7b2', slice_hash
test_source = Path('pc_port/tests/battle_window_constructor_abi_test.c').resolve()
runner = Path('pc_port/tests/run_battle_window_constructor_abi_test.sh').resolve()
(out / 'provenance.json').write_text(json.dumps({
    'scope': 'Typed eight-argument 80032F54 runtime bridge ABI only; no constructor/render oracle.',
    'SLUS_006.64_sha256': sha256(exe).hexdigest(),
    'retail_slice': {'address': hex(start), 'end': hex(end), 'sha256': slice_hash},
    'selected_runtime_source': str(source),
    'source_pins': {
        str(source): sha256(source.read_bytes()).hexdigest(),
        str(test_source): sha256(test_source.read_bytes()).hexdigest(),
        str(runner): sha256(runner.read_bytes()).hexdigest(),
    },
}, indent=2) + '\n')
PY

bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt \
    --out "$OUT/battle_bridge_map.inc"

COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    link_flags=()
    if [ "$mode" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize=function -fno-sanitize-recover=all)
        link_flags=(-fno-sanitize=function)
    fi
    clang "${COMMON[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c \
        -o "$OUT/$mode.compat.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$SOURCE\"" \
        -c pc_port/tests/battle_window_constructor_abi_test.c \
        -o "$OUT/$mode.test.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    clang -no-pie "${flags[@]}" "${link_flags[@]}" -Wl,--gc-sections \
        "$OUT/$mode.compat.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" \
        -ldl -o "$OUT/$mode"
    if ! "$OUT/$mode" > "$OUT/$mode.log" 2>&1; then
        cat "$OUT/$mode.log" >&2
        exit 1
    fi
    cat "$OUT/$mode.log"
done

python3 - "$OUT" "$SOURCE" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys

out = Path(sys.argv[1])
source_path = Path(sys.argv[2]).resolve()
source = source_path.read_text()
start_text = '    if (resolved->address == 0x80032f54u) {\n'
assert source.count(start_text) == 1, 'selected source has no unique constructor bridge block'
start = source.index(start_text)
end = source.index('\n    }\n', start) + len('\n    }\n')
block = source[start:end]
mutants = {
    'missing-shift': block.replace('args[6] = 0;', 'args[6] = (uintptr_t)height;'),
    'wrong-slot': block.replace('cpu->gpr[29] + 0x18u, 2', 'cpu->gpr[29] + 0x1cu, 2'),
    'raw-scalar-translation': block.replace(
        'args[6] = 0;\n        args[7] = (uintptr_t)(intptr_t)(int16_t)height;',
        'args[7] = args[6];\n        args[6] = 0;'),
}
manifest = {}
for name, replacement in mutants.items():
    assert replacement != block
    mutant = source[:start] + replacement + source[end:]
    path = out / (name + '.c')
    path.write_text(mutant)
    manifest[name] = sha256(mutant.encode()).hexdigest()
(out / 'negative-controls.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY

for mutant in missing-shift wrong-slot raw-scalar-translation; do
    clang "${COMMON[@]}" -O2 -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
        -c pc_port/tests/battle_window_constructor_abi_test.c -o "$OUT/$mutant.test.o"
    clang -no-pie -O2 -Wl,--gc-sections \
        "$OUT/O2.compat.o" "$OUT/$mutant.test.o" "$OUT/O2.cpu.o" \
        -ldl -o "$OUT/$mutant"
    if "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "BATTLE WINDOW ABI FAIL negative control survived: $mutant" >&2
        exit 1
    fi
    if ! rg -q 'BATTLE WINDOW ABI FAIL' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        echo "BATTLE WINDOW ABI FAIL negative control lacks semantic failure: $mutant" >&2
        exit 1
    fi
    echo "BATTLE WINDOW ABI negative control rejected: $mutant"
done

python3 - "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys

out = Path(sys.argv[1])
pins = json.loads((out / 'provenance.json').read_text())
for path, expected in pins['source_pins'].items():
    actual = sha256(Path(path).read_bytes()).hexdigest()
    assert actual == expected, f'source changed during test: {path}'
print('BATTLE WINDOW ABI source pins unchanged')
PY
echo 'BATTLE WINDOW ABI test matrix complete'
