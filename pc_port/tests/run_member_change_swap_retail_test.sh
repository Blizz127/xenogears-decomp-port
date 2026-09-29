#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
SCRATCH=/tmp/xeno-member-change-swap-retail-test-20260906
mkdir -p "$SCRATCH"
OUT=$(mktemp -d "$SCRATCH/run.XXXXXXXX")
SOURCE=${MEMBER_CHANGE_SWAP_SOURCE:-src/member_change_menu/main/misc.c}
TEST=pc_port/tests/member_change_swap_retail_test.c
RUNNER=pc_port/tests/run_member_change_swap_retail_test.sh
ADAPTER=pc_port/src/battle_mips_adapter.c
MODULE=disc/member_change_menu.bin
CLANG=/home/linuxbrew/.linuxbrew/bin/clang

echo "MEMBER_CHANGE_SWAP_RETAIL_OUTPUT $OUT"

python3 - "$SOURCE" "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, re, sys

source = Path(sys.argv[1])
out = Path(sys.argv[2])
module = Path('disc/member_change_menu.bin')
test = Path('pc_port/tests/member_change_swap_retail_test.c')
runner = Path('pc_port/tests/run_member_change_swap_retail_test.sh')
adapter = Path('pc_port/src/battle_mips_adapter.c')
header = Path('pc_port/src/battle_mips_adapter.h')
menu_header = Path('include/system/menu.h')
game_header = Path('include/main/game.h')
psyx_config = Path('pc_port/extern/PsyCross/include/PsyX/PsyX_config.h')
build_port = Path('pc_port/build_port.sh')

def digest(path): return sha256(path.read_bytes()).hexdigest()
def function_body(text, name):
    matches = list(re.finditer(r'\b(?:u_char|u_short)\s+' + re.escape(name) +
                               r'\s*\([^;{}]*\)\s*\{',
                               text, re.S))
    if len(matches) != 1:
        raise SystemExit(f'MEMBER_CHANGE_SWAP_RETAIL_RED {name} body count={len(matches)}')
    start = matches[0].start()
    brace = text.index('{', matches[0].start())
    depth = 0
    for pos in range(brace, len(text)):
        if text[pos] == '{': depth += 1
        elif text[pos] == '}':
            depth -= 1
            if depth == 0: return text[start:pos + 1]
    raise SystemExit(f'MEMBER_CHANGE_SWAP_RETAIL_RED unterminated {name}')

if not source.is_file():
    raise SystemExit(f'MEMBER_CHANGE_SWAP_RETAIL_RED missing source {source}')
module_bytes = module.read_bytes()
if len(module_bytes) != 0x6800 or digest(module) != '3b9e2b890c27ae0de97fe343ac75cd35c05fe7f9605ae387d2166da0be78109c':
    raise SystemExit('MEMBER_CHANGE_SWAP_RETAIL_RED member_change_menu.bin pin mismatch')
swap = module_bytes[0x5b48:0x5d14]
helper = module_bytes[0x18:0x34]
masks = module_bytes[0x657c:0x659c]
expected = {
    'swap': (0x1cc, 'a65fe1e11ad3f13b0cd9b732ec90a9a794ba476ef8b09b14ddf0f838a177fbd5'),
    'helper': (0x1c, '8bd5935e3f4bda74214fea110c7420646d7f876b709c9996d882c44d14718d39'),
    'masks': (0x20, '266e8eb790b40fb67ee3415b197d907c10761fe872a2a24cee5cae55063d348c'),
}
for name, data in [('swap', swap), ('helper', helper), ('masks', masks)]:
    size, wanted = expected[name]
    actual = sha256(data).hexdigest()
    if len(data) != size or actual != wanted:
        raise SystemExit(f'MEMBER_CHANGE_SWAP_RETAIL_RED {name} slice size/sha {len(data)}/{actual}')
    (out / f'retail.{name}.bin').write_bytes(data)

source_text = source.read_text()
helper_source = Path('src/member_change_menu/main/misc.c').read_text()
body = function_body(source_text, 'MemberChangeMenuSwapCharacters')
helper_body = function_body(helper_source, 'MemberChangeMenuIsCharacterFlagSet')
helper_actual = helper_body.replace(
    'MemberChangeMenuIsCharacterFlagSet(',
    'MemberChangeMenuIsCharacterFlagSetActual(', 1)
if helper_actual == helper_body:
    raise SystemExit('MEMBER_CHANGE_SWAP_RETAIL_RED helper rename failed')

def instrument_helper(full):
    if full.count(helper_body) != 1:
        raise SystemExit('MEMBER_CHANGE_SWAP_RETAIL_RED helper body instrumentation count')
    return full.replace(helper_body, helper_actual, 1)

specs = {
    'missing-current-lock-check': (
        'MemberChangeMenuIsCharacterFlagSet(*(u16*)((u8*)&g_GameState + 0x2318), g_Menu->pManager->currentCharacterIDs[partyIndex])',
        '(0 && MemberChangeMenuIsCharacterFlagSet(*(u16*)((u8*)&g_GameState + 0x2318), g_Menu->pManager->currentCharacterIDs[partyIndex]))'),
    'missing-bench-lock-check': (
        'MemberChangeMenuIsCharacterFlagSet(*(u16*)((u8*)&g_GameState + 0x2318), g_Menu->unk1E14[benchIndex])',
        '(0 && MemberChangeMenuIsCharacterFlagSet(*(u16*)((u8*)&g_GameState + 0x2318), g_Menu->unk1E14[benchIndex]))'),
    'wrong-party-index': (
        'partyIndex = current;',
        'partyIndex = ((u8)current == 2) ? 1 : (u8)current + 1;'),
    'skipped-swap': (
        'temp = g_Menu->pManager->currentCharacterIDs[partyIndex];\n'
        '        g_Menu->pManager->currentCharacterIDs[partyIndex] = g_Menu->unk1E14[benchIndex];\n'
        '        g_Menu->unk1E14[benchIndex] = temp;\n'
        '        i = 0;',
        'temp = g_Menu->pManager->currentCharacterIDs[partyIndex];\n'
        '        g_Menu->pManager->currentCharacterIDs[partyIndex] = temp;\n'
        '        g_Menu->unk1E14[benchIndex] = temp;\n'
        '        i = 0;'),
    'omitted-rollback': ('if (!count) {', 'if (0) {'),
    'count-only-two-party-slots': ('for (; i < 3; i++) {', 'for (; i < 2; i++) {'),
    'wrong-success-result': ('result = 1;', 'result = 0;'),
}
for name, (old, new) in specs.items():
    count = body.count(old)
    if count != 1:
        raise SystemExit(f'MEMBER_CHANGE_SWAP_RETAIL_RED control {name} anchor count={count}')
    mutant_body = body.replace(old, new, 1)
    full = source_text[:source_text.index(body)] + mutant_body + source_text[source_text.index(body) + len(body):]
    (out / f'control-{name}.c').write_text(instrument_helper(full))

(out / 'production.body.c').write_text(body + '\n')
(out / 'production.helper.c').write_text(helper_body + '\n')
(out / 'production.instrumented.c').write_text(instrument_helper(source_text))
(out / 'pins.json').write_text(json.dumps({
    'source': str(source), 'source_sha256': digest(source),
    'source_body_sha256': sha256(body.encode()).hexdigest(),
    'helper_source': 'src/member_change_menu/main/misc.c',
    'helper_source_sha256': sha256(helper_source.encode()).hexdigest(),
    'helper_body_sha256': sha256(helper_body.encode()).hexdigest(),
    'module': str(module), 'module_sha256': digest(module),
    'retail_range': 'member_change_menu.bin[0x5B48:0x5D14] / 0x801CAB48..0x801CAD14',
    'retail_swap_sha256': sha256(swap).hexdigest(),
    'retail_helper_range': 'member_change_menu.bin[0x18:0x34] / 0x801C5018..0x801C5034',
    'retail_helper_sha256': sha256(helper).hexdigest(),
    'retail_masks_range': 'member_change_menu.bin[0x657C:0x659C] / 0x801CB57C..0x801CB59C',
    'retail_masks_sha256': sha256(masks).hexdigest(),
    'test_sha256': digest(test), 'runner_sha256': digest(runner),
    'adapter_sha256': digest(adapter), 'adapter_header_sha256': digest(header),
    'menu_header_sha256': digest(menu_header),
    'game_header_sha256': digest(game_header),
    'psyx_config_sha256': digest(psyx_config),
    'build_port_sha256': digest(build_port),
    'native_compile_definitions': [
        'XENO_PC_PORT', 'XENO_FIELD_OBJECT_OVERLAY', 'SKIP_ASM',
        '_LANGUAGE_C', 'USE_EXTENDED_PRIM_POINTERS=0'],
    'native_game_state_storage': 'fixture symbol is 0x4600 bytes, matching pc_port/src/data_game_state.c',
    'native_layout': 'actual SystemMenu/MenuManager host structs; guest side uses retail packed offsets',
    'controls': list(specs),
}, indent=2) + '\n')
PY

common=(
    -std=gnu17 -g -fno-pie -fno-builtin -ffunction-sections -fdata-sections
    -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/src -Ipc_port/include_shim -Iinclude
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
    -include assert.h
    -Wno-implicit-function-declaration -Wno-int-conversion
    -Wno-incompatible-pointer-types
)

build_run() {
    local label=$1 compiler=$2 opt=$3 source_file=$4 expect=$5
    local -a flags=("-$opt") sanitize=()
    if [[ "$opt" == UBSan ]]; then
        flags=(-O1)
        sanitize=(-fsanitize=undefined -fno-sanitize-recover=all)
    fi
    "$compiler" "${common[@]}" "${flags[@]}" "${sanitize[@]}" -c "$source_file" \
        -o "$OUT/$label.production.o" >"$OUT/$label.build.log" 2>&1
    if ! nm "$OUT/$label.production.o" | rg -q ' T MemberChangeMenuIsCharacterFlagSetActual$'; then
        echo "MEMBER_CHANGE_SWAP_RETAIL_FAIL $label renamed actual helper missing" \
            >>"$OUT/$label.build.log"
        cat "$OUT/$label.build.log" >&2
        exit 1
    fi
    if ! nm "$OUT/$label.production.o" | rg -q ' U MemberChangeMenuIsCharacterFlagSet$'; then
        echo "MEMBER_CHANGE_SWAP_RETAIL_FAIL $label target does not reference traced helper" \
            >>"$OUT/$label.build.log"
        cat "$OUT/$label.build.log" >&2
        exit 1
    fi
    "$compiler" "${common[@]}" "${flags[@]}" "${sanitize[@]}" -c "$TEST" \
        -o "$OUT/$label.test.o" >>"$OUT/$label.build.log" 2>&1
    "$compiler" "${common[@]}" "${flags[@]}" "${sanitize[@]}" -c "$ADAPTER" \
        -o "$OUT/$label.adapter.o" >>"$OUT/$label.build.log" 2>&1
    "$compiler" -no-pie "${flags[@]}" "${sanitize[@]}" -Wl,--gc-sections \
        "$OUT/$label.production.o" "$OUT/$label.test.o" "$OUT/$label.adapter.o" \
        -o "$OUT/$label.test" >>"$OUT/$label.build.log" 2>&1
    set +e
    "$OUT/$label.test" "$OUT/retail.swap.bin" "$OUT/retail.helper.bin" \
        "$OUT/retail.masks.bin" >"$OUT/$label.log" 2>&1
    local rc=$?
    set -e
    if [[ "$expect" == PASS ]]; then
        if [[ $rc -ne 0 ]] || ! rg -q '^MEMBER_CHANGE_SWAP_RETAIL_PASS ' "$OUT/$label.log"; then
            cat "$OUT/$label.build.log" "$OUT/$label.log" >&2
            echo "MEMBER_CHANGE_SWAP_RETAIL_FAIL $label expected PASS rc=$rc" >&2
            exit 1
        fi
    else
        if [[ $rc -ne 1 ]] || ! rg -q '^MEMBER_CHANGE_SWAP_RETAIL_FAIL ' "$OUT/$label.log"; then
            cat "$OUT/$label.build.log" "$OUT/$label.log" >&2
            echo "MEMBER_CHANGE_SWAP_RETAIL_FAIL $label control expected rc=1/FAIL, got $rc" >&2
            exit 1
        fi
    fi
    cat "$OUT/$label.log"
}

build_run production-O0 gcc O0 "$OUT/production.instrumented.c" PASS
build_run production-O2 gcc O2 "$OUT/production.instrumented.c" PASS
build_run production-UBSan "$CLANG" UBSan "$OUT/production.instrumented.c" PASS

for control in missing-current-lock-check missing-bench-lock-check wrong-party-index \
               skipped-swap omitted-rollback count-only-two-party-slots \
               wrong-success-result; do
    build_run "control-$control" gcc O2 "$OUT/control-$control.c" FAIL
done

python3 - "$SOURCE" "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, subprocess, sys

source, out = Path(sys.argv[1]), Path(sys.argv[2])
pins = json.loads((out / 'pins.json').read_text())
checks = {
    source: pins['source_sha256'],
    Path('src/member_change_menu/main/misc.c'): pins['helper_source_sha256'],
    Path('disc/member_change_menu.bin'): pins['module_sha256'],
    Path('pc_port/tests/member_change_swap_retail_test.c'): pins['test_sha256'],
    Path('pc_port/tests/run_member_change_swap_retail_test.sh'): pins['runner_sha256'],
    Path('pc_port/src/battle_mips_adapter.c'): pins['adapter_sha256'],
    Path('pc_port/src/battle_mips_adapter.h'): pins['adapter_header_sha256'],
    Path('include/system/menu.h'): pins['menu_header_sha256'],
    Path('include/main/game.h'): pins['game_header_sha256'],
    Path('pc_port/extern/PsyCross/include/PsyX/PsyX_config.h'): pins['psyx_config_sha256'],
    Path('pc_port/build_port.sh'): pins['build_port_sha256'],
}
for path, wanted in checks.items():
    actual = sha256(path.read_bytes()).hexdigest()
    if actual != wanted:
        raise SystemExit(f'MEMBER_CHANGE_SWAP_RETAIL_FAIL changed during run {path}: {actual} != {wanted}')
versions = {}
for compiler in ('gcc', '/home/linuxbrew/.linuxbrew/bin/clang'):
    versions[compiler] = subprocess.check_output([compiler, '--version'], text=True).splitlines()[0]
pins['compiler_manifest'] = versions
pins['results'] = ['production-O0 PASS', 'production-O2 PASS', 'production-UBSan PASS',
                   '7 semantic controls rejected']
(out / 'manifest.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

echo "MEMBER_CHANGE_SWAP_RETAIL_GREEN O0/O2/ClangUBSan 1389 cases + 80 helper cases; 7 controls; evidence=$OUT"
