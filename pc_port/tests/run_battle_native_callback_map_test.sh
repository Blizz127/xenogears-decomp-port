#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/battle_native_callback_map_test.XXXXXXXX)
export OUT
echo "NATIVE CALLBACK artifacts: $OUT"

bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --out "$OUT/battle_bridge_map.inc"

python3 - "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os, sys
out=Path(sys.argv[1])
paths=[
 'pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c',
 'tools/scripts/gen_battle_bridge_map.py','config/symbol_addrs.slus_006.64.txt',
 'linker/undefined_funcs_auto.battle.txt','linker/undefined_syms_auto.battle.txt',
 'pc_port/tests/battle_native_callback_map_test.c',
 'pc_port/tests/run_battle_native_callback_map_test.sh']
pins={
 'source_files':{p:sha256(Path(p).read_bytes()).hexdigest() for p in paths},
 'generated_bridge_map_sha256':sha256((out/'battle_bridge_map.inc').read_bytes()).hexdigest(),
 'ubsan_note':'UBSan runs use -fno-sanitize=function because runtime_bridge_call intentionally invokes the generic host-function ABI through a variadic-width function pointer.',
 'scope':'Map/ABI boundary only: func_80022DF4/func_80022E8C/func_80022EB8 native host resolution, translated guest data, unknown-host rejection, generated-stub rejection, and spy callbacks; no retail tick semantics.'}
(out/'provenance.json').write_text(json.dumps(pins,indent=2)+'\n')
(out/'stub.c').write_text('''#include <string.h>\nint reject_generated_stub;\nint xeno_port_is_generated_stub(const char *name) {\n    return reject_generated_stub && strcmp(name, "func_80022DF4") == 0;\n}\n''')
PY

common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
    -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

build_case() {
    local name=$1 opt=$2
    local -a flags
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize=function -fno-sanitize-recover=all)
    fi
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/battle_native_callback_map_test.c -o "$OUT/$name.test.o" > "$OUT/$name.build.log" 2>&1
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$name.cpu.o" >> "$OUT/$name.build.log" 2>&1
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c "$OUT/stub.c" -o "$OUT/$name.stub.o" >> "$OUT/$name.build.log" 2>&1
    # battle_mips_runtime.c calls PcPort_GodModeBeforeGuest() on every guest
    # entry, so god_mode.c has to be in the link or the fixture fails to build.
    clang "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/god_mode.c -o "$OUT/$name.godmode.o" >> "$OUT/$name.build.log" 2>&1
    clang -no-pie "${flags[@]}" -Wl,--gc-sections -Wl,--export-dynamic \
        "$OUT/$name.test.o" "$OUT/$name.cpu.o" "$OUT/$name.stub.o" \
        "$OUT/$name.godmode.o" \
        -ldl -o "$OUT/$name.test" \
        >> "$OUT/$name.build.log" 2>&1
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

red=0
for opt in O0 O2 UBSan; do
    if ! build_case "$opt" "$opt"; then
        cat "$OUT/$opt.build.log" >&2
        echo "NATIVE CALLBACK INFRASTRUCTURE FAILURE: $opt" >&2
        exit 2
    fi
    status=0
    "$OUT/$opt.test" > "$OUT/$opt.log" 2>&1 || status=$?
    if [ "$status" -eq 0 ]; then
        cat "$OUT/$opt.log"
    elif rg -q '^NATIVE CALLBACK RED missing map entry' "$OUT/$opt.log"; then
        cat "$OUT/$opt.log" >&2
        red=1
    else
        cat "$OUT/$opt.log" >&2
        echo "NATIVE CALLBACK UNEXPECTED FAILURE: $opt rc=$status" >&2
        exit 1
    fi
done
verify_source_pins
if [ "$red" -ne 0 ]; then
    echo "NATIVE CALLBACK RED: func_80022DF4 is absent from generated map" >&2
    exit 1
fi
echo "NATIVE CALLBACK GREEN: O0/O2/UBSan (function-type sanitizer excluded for generic bridge ABI) map/ABI boundary; $OUT"
