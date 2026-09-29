#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
TEST_OUT=$(mktemp -d pc_port/build_native/sprite_dispatch_aa_retail_test.XXXXXXXX)
export TEST_OUT
printf 'SPRITE AA artifacts: %s\n' "$TEST_OUT"

python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os, struct
out = Path(os.environ['TEST_OUT']); disc = Path('disc/SLUS_006.64').read_bytes(); body_source = os.environ.get('SPRITE_AA_SOURCE', 'src/slus_006.64/system/animation_scripts.c')
assert sha256(disc).hexdigest() == 'dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
pins = {}
for name, start, end, digest in [
    ('dispatcher', 0x8001fbe4, 0x80021ad8, '7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c'),
    ('handler', 0x800216f4, 0x80021730, '034b8d78485a7cfcec005072a33950381ee5cb048b0113b266e692cddfbc5938'),
    ('helper', 0x80022cac, 0x80022cdc, '7edc9e76b551fe6d2b564c8ba92beb0785e2426f125dcc3695d928640f42f3c0')]:
    actual = sha256(disc[start - 0x8000f800:end - 0x8000f800]).hexdigest(); assert actual == digest, name
    pins[name] = {'start': hex(start), 'end': hex(end), 'sha256': actual}
assert struct.unpack_from('<I', disc, 0x800183d8 + (0xaa - 0x8a) * 4 - 0x8000f800)[0] == 0x800216f4
for path in [body_source, 'pc_port/src/game_overrides.c', 'src/slus_006.64/system/temp1.c', 'pc_port/src/battle_mips_adapter.c', 'pc_port/tests/sprite_dispatch_aa_retail_test.c', 'pc_port/tests/run_sprite_dispatch_aa_retail_test.sh']:
    data = Path(path).read_bytes(); pins[path] = sha256(data).hexdigest(); (out / Path(path).name).write_bytes(data)
source = Path('src/slus_006.64/system/temp1.c').read_text(); start = source.index('s32 func_80022CAC(')
depth = 0; end = None
for i in range(source.index('{', start), len(source)):
    if source[i] == '{': depth += 1
    elif source[i] == '}':
        depth -= 1
        if depth == 0: end = i + 1; break
assert end is not None
helper = source[start:end]
(out / 'aa_helper.c').write_text('#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef int32_t s32; typedef int64_t s64;\n' + helper.replace('func_80022CAC(', 'aa_helper_impl(', 1) + '\n')
pins['helper_source_sha256'] = sha256(helper.encode()).hexdigest()
pins['scope'] = 'Actual retail AA handler and retail 80022CAC MIPS helper; native helper is verbatim source-pinned extraction; no renderer claim.'
(out / 'provenance.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

build_case() {
    local name=$1 opt=$2 source=${3:-${SPRITE_AA_SOURCE:-src/slus_006.64/system/animation_scripts.c}}
    local -a flags common
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include assert.h -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
    gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/src -c "$source" -o "$TEST_OUT/$name.body.o" > "$TEST_OUT/$name.build.log" 2>&1
    gcc -std=gnu17 -fpermissive "${common[@]}" -Ipc_port/src -c "$TEST_OUT/aa_helper.c" -o "$TEST_OUT/$name.helper.o" >> "$TEST_OUT/$name.build.log" 2>&1
    clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_aa_retail_test.c pc_port/src/battle_mips_adapter.c "$TEST_OUT/$name.body.o" "$TEST_OUT/$name.helper.o" -o "$TEST_OUT/$name.test" >> "$TEST_OUT/$name.build.log" 2>&1
}

if [ "${SPRITE_AA_RUN_GREEN:-1}" != 1 ]; then
    for opt in O0 O2 UBSan; do
        build_case "$opt" "$opt"
        status=0; "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1 || status=$?
        if [ "$status" -eq 0 ] || ! rg -q 'sprite_animation_unimplemented.*"opcode":170' "$TEST_OUT/$opt.log"; then
            cat "$TEST_OUT/$opt.log" >&2; printf 'SPRITE AA UNEXPECTED RESULT %s rc=%s\n' "$opt" "$status" >&2; exit 2
        fi
        printf 'SPRITE AA %s semantic RED: native dispatcher rejects opcode 0xAA\n' "$opt"
    done
fi
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path: assert sha256(Path(path).read_bytes()).hexdigest() == expected, path
PY
if [ "${SPRITE_AA_RUN_GREEN:-1}" = 1 ]; then
    for opt in O0 O2 UBSan; do
        build_case "$opt" "$opt"
        "$TEST_OUT/$opt.test" > "$TEST_OUT/$opt.log" 2>&1
        cat "$TEST_OUT/$opt.log"
    done
python3 - "$TEST_OUT" <<'PY'
from pathlib import Path
import sys, os
out = Path(sys.argv[1])
source = Path(os.environ.get('SPRITE_AA_SOURCE', 'src/slus_006.64/system/animation_scripts.c')).read_text()
a = source.index('    case 0xAA:'); b = source.index('    case 0xE7:', a); body = source[a:b]
mutants = {
    'unsigned-operand': body.replace('(s8)((u8*)operands)[0]', '((u8*)operands)[0]'),
    'wrong-scale-field': body.replace('p + 0x2C', 'p + 0x2E'),
    'missing-negative-round': body.replace('        if (product < 0) product += 0xFFF;\n', ''),
    'wrong-helper-shift': body.replace('product >> 12', 'product >> 11'),
    'wrong-position-load': body.replace('AnimationRead32(p + 4)', 'AnimationRead32(p + 8)'),
    'missing-helper': body.replace('displacement = func_80022CAC(p, product >> 12);', 'displacement = product >> 12;'),
}
for name, changed in mutants.items():
    assert changed != body, name
    (out / (name + '.c')).write_text(source[:a] + changed + source[b:])
PY
    for kind in unsigned-operand wrong-scale-field missing-negative-round wrong-helper-shift wrong-position-load missing-helper; do
        build_case "control_$kind" O2 "$TEST_OUT/$kind.c"
        status=0
        SPRITE_AA_QUICK=1 SPRITE_AA_OPERAND=128 SPRITE_AA_SCALE=32767 SPRITE_AA_TIMER=1023 SPRITE_AA_ALIAS=0 "$TEST_OUT/control_$kind.test" > "$TEST_OUT/control_$kind.log" 2>&1 || status=$?
        if [ "$status" -eq 0 ] || ! rg -q '^SPRITE AA FAIL ' "$TEST_OUT/control_$kind.log"; then
            cat "$TEST_OUT/control_$kind.log" >&2; printf 'SPRITE AA CONTROL SURVIVED: %s\n' "$kind" >&2; exit 1
        fi
        printf 'SPRITE AA control rejected %s: %s\n' "$kind" "$(rg -m1 '^SPRITE AA FAIL ' "$TEST_OUT/control_$kind.log")"
    done
    python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import json, os
pins = json.loads((Path(os.environ['TEST_OUT']) / 'provenance.json').read_text())
for path, expected in pins.items():
    if '/' in path: assert sha256(Path(path).read_bytes()).hexdigest() == expected, path
PY
    printf 'SPRITE AA GREEN: O0/O2/UBSan full domains and six isolated controls; %s\n' "$TEST_OUT"
    exit 0
fi
printf 'SPRITE AA RED: retail handler/helper fixture is runnable; native AA remains unsupported\n'
