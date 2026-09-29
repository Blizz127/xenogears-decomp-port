#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/battle_sprite_callback_retail_test.XXXXXXXX)
export OUT
echo "CALLBACK artifacts: $OUT"

bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt \
    --out "$OUT/battle_bridge_map.inc"

python3 - "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, sys
out = Path(sys.argv[1])
slus = Path('disc/SLUS_006.64').read_bytes()
assert sha256(slus).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
base = 0x8000f800
setter = slus[0x80021bf8-base:0x80021c00-base]
assert sha256(setter).hexdigest() == '1e245fc14172e83f63328ec67268d1c88f1551d03edfabd775a52b067f500fba'
source = Path('src/slus_006.64/system/animation_scripts.c').read_text()
start = source.index('void func_80021BF8(void* arg0, s32 arg1) {')
end = source.index('\n}', start) + 2
body = source[start:end]
(out/'setter.c').write_text('#include <stdint.h>\ntypedef uint8_t u8; typedef int32_t s32;\n'+body+'\n')
paths = [
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c',
 'src/slus_006.64/system/animation_scripts.c',
 'tools/scripts/gen_battle_bridge_map.py','config/symbol_addrs.slus_006.64.txt',
 'linker/undefined_funcs_auto.battle.txt','linker/undefined_syms_auto.battle.txt',
 'config/symbol_addrs.battle.txt','pc_port/tests/battle_sprite_callback_retail_test.c',
 'pc_port/tests/run_battle_sprite_callback_retail_test.sh']
pins = {'SLUS_006.64':sha256(slus).hexdigest(),
 'setter':{'start':hex(0x80021bf8),'end':hex(0x80021c00),'sha256':sha256(setter).hexdigest()},
 'setter_source_sha256':sha256(body.encode()).hexdigest(),
 'source_files':{p:sha256(Path(p).read_bytes()).hexdigest() for p in paths},
 'generated_bridge_map_sha256':sha256((out/'battle_bridge_map.inc').read_bytes()).hexdigest(),
 'ubsan_note':'UBSan runs use -fno-sanitize=function because runtime_bridge_call intentionally invokes the existing generic host-function ABI through a variadic-width function pointer.',
 'scope':'Actual SLUS setter through MIPS adapter, actual production runtime classification/bridge call, translated sprite data, guest callback preservation, zero/native values, and relocated pinned retail callback.'}
(out/'provenance.json').write_text(json.dumps(pins,indent=2)+'\n')
PY

common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

build_case() {
    local name=$1 opt=$2 source=${3:-pc_port/tests/battle_sprite_callback_retail_test.c}
    local -a flags
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize=function -fno-sanitize-recover=all)
    fi
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "$source" -o "$OUT/$name.test.o" > "$OUT/$name.build.log" 2>&1
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$name.cpu.o" >> "$OUT/$name.build.log" 2>&1
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c "$OUT/setter.c" -o "$OUT/$name.setter.o" >> "$OUT/$name.build.log" 2>&1
    clang -no-pie "${flags[@]}" -Wl,--gc-sections -Wl,--export-dynamic "$OUT/$name.test.o" "$OUT/$name.cpu.o" "$OUT/$name.setter.o" -ldl -o "$OUT/$name.test" >> "$OUT/$name.build.log" 2>&1
}

verify_source_pins() {
    python3 - <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os
pins=json.loads((Path(os.environ['OUT'])/'provenance.json').read_text())
for path,expected in pins['source_files'].items():
    assert sha256(Path(path).read_bytes()).hexdigest()==expected, 'Source changed during test: '+path
PY
}

failed=0
semantic_red=0
for opt in O0 O2 UBSan; do
    if ! build_case "$opt" "$opt"; then cat "$OUT/$opt.build.log" >&2; echo "CALLBACK INFRASTRUCTURE FAILURE: $opt" >&2; exit 2; fi
    status=0
    "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        cat "$OUT/$opt.log"
    elif rg -q '^CALLBACK RED classification omitted:' "$OUT/$opt.log"; then
        cat "$OUT/$opt.log" >&2
        semantic_red=1
    else
        cat "$OUT/$opt.log" >&2
        echo "CALLBACK UNEXPECTED FAILURE: $opt rc=$status" >&2
        exit 1
    fi
done
if [ "$semantic_red" -ne 0 ]; then
    verify_source_pins
    echo "CALLBACK RED: production callback classification is not installed" >&2
    exit 1
fi

make_classification_mutant() {
    python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
out=Path(sys.argv[1])
source=Path('pc_port/src/battle_mips_runtime.c').read_text()
lines=source.splitlines(keepends=True)
hits=[i for i,line in enumerate(lines) if 'strcmp(name, "func_80021BF8")' in line]
if len(hits)!=1: raise SystemExit(f'callback classification mutation matched {len(hits)} lines')
lines.pop(hits[0])
(out/'runtime_no_callback.c').write_text(''.join(lines))
test=Path('pc_port/tests/battle_sprite_callback_retail_test.c').read_text()
needle='#include "../src/battle_mips_runtime.c"'
if test.count(needle)!=1: raise SystemExit('runtime include mutation site is not unique')
(out/'test_no_callback.c').write_text(test.replace(needle,'#include "runtime_no_callback.c"',1))
PY
}

make_classification_mutant
if ! build_case no_callback O2 "$OUT/test_no_callback.c"; then
    cat "$OUT/no_callback.build.log" >&2
    echo "CALLBACK CONTROL INFRASTRUCTURE FAILURE" >&2
    exit 2
fi
if "$OUT/no_callback.test" > "$OUT/no_callback.log" 2>&1; then
    echo "CALLBACK CONTROL FAILED TO REJECT classification omission" >&2
    failed=1
elif rg -q '^CALLBACK RED classification omitted:' "$OUT/no_callback.log"; then
    echo "CALLBACK CONTROL PASS: classification omission rejected"
else
    cat "$OUT/no_callback.log" >&2
    echo "CALLBACK CONTROL UNEXPECTED FAILURE" >&2
    exit 2
fi

verify_source_pins
if [ "$failed" -ne 0 ]; then exit 1; fi
echo "CALLBACK GREEN: O0/O2/UBSan (function-type sanitizer excluded for generic bridge ABI) retail setter/bridge/callback regression; $OUT"
