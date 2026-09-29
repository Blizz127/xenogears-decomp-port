#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
SCRATCH=/tmp/xeno-member-change-name-retail-test-20260906
mkdir -p "$SCRATCH"
OUT=$(mktemp -d "$SCRATCH/run.XXXXXXXX")
SOURCE=${MEMBER_CHANGE_NAME_SOURCE:-src/member_change_menu/main/misc.c}
TEST=pc_port/tests/member_change_name_retail_test.c
RUNNER=pc_port/tests/run_member_change_name_retail_test.sh
ADAPTER=pc_port/src/battle_mips_adapter.c
DATA=pc_port/src/data_member_change_menu.c
MODULE=disc/member_change_menu.bin
CLANG="${CLANG:-$(command -v clang || echo /home/linuxbrew/.linuxbrew/bin/clang)}"; export CLANG

echo "MEMBER_CHANGE_NAME_RETAIL_OUTPUT $OUT"

python3 - "$SOURCE" "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, re, sys

source = Path(sys.argv[1])
out = Path(sys.argv[2])
module = Path('disc/member_change_menu.bin')
test = Path('pc_port/tests/member_change_name_retail_test.c')
runner = Path('pc_port/tests/run_member_change_name_retail_test.sh')
adapter = Path('pc_port/src/battle_mips_adapter.c')
adapter_header = Path('pc_port/src/battle_mips_adapter.h')
data = Path('pc_port/src/data_member_change_menu.c')
menu_header = Path('include/system/menu.h')
game_header = Path('include/main/game.h')
psyx_config = Path('pc_port/extern/PsyCross/include/PsyX/PsyX_config.h')
build_port = Path('pc_port/build_port.sh')

def digest(path):
    return sha256(path.read_bytes()).hexdigest()

def function_body(text, name):
    matches = list(re.finditer(r'\bvoid\s+' + re.escape(name) +
                               r'\s*\([^;{}]*\)\s*\{', text, re.S))
    if len(matches) != 1:
        raise SystemExit(f'MEMBER_CHANGE_NAME_RETAIL_RED {name} body count={len(matches)}')
    start = matches[0].start()
    brace = text.index('{', matches[0].start())
    depth = 0
    for pos in range(brace, len(text)):
        if text[pos] == '{':
            depth += 1
        elif text[pos] == '}':
            depth -= 1
            if depth == 0:
                return text[start:pos + 1]
    raise SystemExit(f'MEMBER_CHANGE_NAME_RETAIL_RED unterminated {name}')

expected_files = {
    module: '3b9e2b890c27ae0de97fe343ac75cd35c05fe7f9605ae387d2166da0be78109c',
}
for path, wanted in expected_files.items():
    if not path.is_file() or digest(path) != wanted:
        raise SystemExit(f'MEMBER_CHANGE_NAME_RETAIL_RED pin mismatch {path}')

module_bytes = module.read_bytes()
if len(module_bytes) != 0x6800:
    raise SystemExit('MEMBER_CHANGE_NAME_RETAIL_RED module size')
slices = {
    'function': module_bytes[0x45a0:0x469c],
    'table-u': module_bytes[0x6344:0x6390],
    'table-v': module_bytes[0x6390:0x63dc],
}
expected_slices = {
    'function': (0xfc, '29febe445880a710e2005e11daed43370fa7b9db27f7fe32b778a7a240ef76c7'),
    'table-u': (0x4c, '1f869095081bd29c943be85abaa4091172e0cf663e4907f3b0135027d76ccbeb'),
    'table-v': (0x4c, '7832ee2e7dbd0043991e0ffd3d38da5cb4c31382cea0c26205c5c9b9903706d0'),
}
for name, payload in slices.items():
    size, wanted = expected_slices[name]
    actual = sha256(payload).hexdigest()
    if len(payload) != size or actual != wanted:
        raise SystemExit(f'MEMBER_CHANGE_NAME_RETAIL_RED {name} size/sha {len(payload)}/{actual}')
    (out / f'retail-{name}.bin').write_bytes(payload)

source_text = source.read_text()
body = function_body(source_text, 'func_801C95A0')
schedule = [
    body.index('HeapAlloc(0x3F6, 0)'),
    body.index('bzero(buf, 0x3F6)'),
    body.index('off = (((u32)charByte & 0xFF) >> 1) * 0x28;'),
    body.index('SystemRenderStringEntry((u8*)&g_GameState + off, buf, 0x24, 0);'),
    body.index('s2 = ((u32)slot << 1) & 0x1FC;'),
    body.index('LoadImage(&rect, (u_long*)buf);'),
    body.index('DrawSync(0);'),
    body.index('HeapFree(buf);'),
]
if schedule != sorted(schedule) or len(set(schedule)) != len(schedule):
    raise SystemExit('MEMBER_CHANGE_NAME_RETAIL_RED production scheduling/order gate')
controls = {
    'wrong-character-offset': (
        'off = (((u32)charByte & 0xFF) >> 1) * 0x28;',
        'off = (((u32)charByte & 0xFF) >> 0) * 0x28;'),
    'wrong-second-render-flag': (
        'SystemRenderStringEntry((u8*)&g_GameState + off + 0x14, buf, 0x24, 1);',
        'SystemRenderStringEntry((u8*)&g_GameState + off + 0x14, buf, 0x24, 0);'),
    'wrong-image-dimensions': ('rect.w = 0x28;', 'rect.w = 0x27;'),
    'wrong-table-index': (
        's2 = ((u32)slot << 1) & 0x1FC;',
        's2 = (((u32)slot << 1) + 2) & 0x1FC;'),
}
body_at = source_text.index(body)
for name, (old, new) in controls.items():
    if body.count(old) != 1:
        raise SystemExit(f'MEMBER_CHANGE_NAME_RETAIL_RED control anchor {name} count={body.count(old)}')
    mutant = body.replace(old, new, 1)
    full = source_text[:body_at] + mutant + source_text[body_at + len(body):]
    (out / f'control-{name}.c').write_text(full)

signed_old = 's2 = ((u32)slot << 1) & 0x1FC;'
signed_reconstructed = 's2 = (slot << 1) & 0x1FC;'
if body.count(signed_old) != 1:
    raise SystemExit('MEMBER_CHANGE_NAME_RETAIL_RED reconstructed signed-shift anchor')
signed_body = body.replace(signed_old, signed_reconstructed, 1)
(out / 'control-reconstructed-signed-slot-shift.c').write_text(
    source_text[:body_at] + signed_body + source_text[body_at + len(body):])

(out / 'production.body.c').write_text(body + '\n')
(out / 'pins.json').write_text(json.dumps({
    'source': str(source),
    'source_sha256': digest(source),
    'source_body_sha256': sha256(body.encode()).hexdigest(),
    'module_sha256': digest(module),
    'retail_range': 'member_change_menu.bin[0x45A0:0x469C] / 0x801C95A0..0x801C969C',
    'retail_function_sha256': sha256(slices['function']).hexdigest(),
    'retail_table_u_range': 'member_change_menu.bin[0x6344:0x6390] / 0x801CB344..0x801CB390',
    'retail_table_u_sha256': sha256(slices['table-u']).hexdigest(),
    'retail_table_v_range': 'member_change_menu.bin[0x6390:0x63DC] / 0x801CB390..0x801CB3DC',
    'retail_table_v_sha256': sha256(slices['table-v']).hexdigest(),
    'test_sha256': digest(test),
    'runner_sha256': digest(runner),
    'adapter_sha256': digest(adapter),
    'adapter_header_sha256': digest(adapter_header),
    'data_member_change_menu_sha256': digest(data),
    'menu_header_sha256': digest(menu_header),
    'game_header_sha256': digest(game_header),
    'psyx_config_sha256': digest(psyx_config),
    'build_port_sha256': digest(build_port),
    'native_compile_definitions': [
        'XENO_PC_PORT', 'XENO_FIELD_OBJECT_OVERLAY', 'SKIP_ASM',
        '_LANGUAGE_C', 'USE_EXTENDED_PRIM_POINTERS=0'],
    'source_schedule_gate': [
        'HeapAlloc', 'bzero', 'character offset', 'first render',
        'slot table offset', 'LoadImage', 'DrawSync', 'HeapFree'],
    'controls': list(controls),
    'reconstructed_ubsan_control': (
        'one asserted site: ((u32)slot << 1) -> (slot << 1); '
        'reconstructs the former signed-shift defect from the current full TU'),
    'scope': ('actual full misc.c target with gc-sections and actual native table data; '
              'raw 252-byte retail oracle; boundary mocks, not real font/GPU rendering'),
}, indent=2) + '\n')
PY

COMMON=(
    -std=gnu17 -g -fno-pie -fno-builtin -U_FORTIFY_SOURCE -ffunction-sections -fdata-sections
    -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h
    -Ipc_port/src -Ipc_port/include_shim -Iinclude
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
    -Wno-implicit-function-declaration -Wno-int-conversion
    -Wno-incompatible-pointer-types -Wno-deprecated-non-prototype
)

build_objects() {
    local label=$1 compiler=$2 opt=$3 source_file=$4
    local -a flags=("-$opt") sanitize=()
    if [[ "$opt" == UBSan ]]; then
        flags=(-O1)
        sanitize=(-fsanitize=undefined -fno-sanitize-recover=all)
    fi
    "$compiler" "${COMMON[@]}" "${flags[@]}" "${sanitize[@]}" \
        -c "$source_file" -o "$OUT/$label.source.o" \
        >"$OUT/$label.build.log" 2>&1
    "$compiler" "${COMMON[@]}" "${flags[@]}" "${sanitize[@]}" \
        -c "$DATA" -o "$OUT/$label.data.o" \
        >>"$OUT/$label.build.log" 2>&1
    "$compiler" "${COMMON[@]}" "${flags[@]}" "${sanitize[@]}" \
        -c "$TEST" -o "$OUT/$label.test.o" \
        >>"$OUT/$label.build.log" 2>&1
    "$compiler" "${COMMON[@]}" "${flags[@]}" "${sanitize[@]}" \
        -c "$ADAPTER" -o "$OUT/$label.adapter.o" \
        >>"$OUT/$label.build.log" 2>&1
    "$compiler" -no-pie "${flags[@]}" "${sanitize[@]}" -Wl,--gc-sections \
        "$OUT/$label.source.o" "$OUT/$label.data.o" \
        "$OUT/$label.test.o" "$OUT/$label.adapter.o" \
        -o "$OUT/$label.test" >>"$OUT/$label.build.log" 2>&1
}

run_expected() {
    local label=$1 expectation=$2
    set +e
    "$OUT/$label.test" "$OUT/retail-function.bin" \
        "$OUT/retail-table-u.bin" "$OUT/retail-table-v.bin" \
        >"$OUT/$label.log" 2>&1
    local rc=$?
    set -e
    if [[ "$expectation" == PASS ]]; then
        if [[ $rc -ne 0 ]] || ! rg -q '^MEMBER_CHANGE_NAME_RETAIL_PASS ' "$OUT/$label.log"; then
            cat "$OUT/$label.build.log" "$OUT/$label.log" >&2
            echo "MEMBER_CHANGE_NAME_RETAIL_FAIL $label expected PASS rc=$rc" >&2
            exit 1
        fi
    else
        if [[ $rc -ne 1 ]] || ! rg -q '^MEMBER_CHANGE_NAME_RETAIL_FAIL ' "$OUT/$label.log"; then
            cat "$OUT/$label.build.log" "$OUT/$label.log" >&2
            echo "MEMBER_CHANGE_NAME_RETAIL_FAIL $label control expected rc1/FAIL got $rc" >&2
            exit 1
        fi
    fi
    cat "$OUT/$label.log"
}

build_objects production-O0 gcc O0 "$SOURCE"
run_expected production-O0 PASS
build_objects production-O2 gcc O2 "$SOURCE"
run_expected production-O2 PASS
build_objects production-UBSan "$CLANG" UBSan "$SOURCE"
run_expected production-UBSan PASS

for control in wrong-character-offset wrong-second-render-flag \
               wrong-image-dimensions wrong-table-index; do
    build_objects "control-$control" gcc O2 "$OUT/control-$control.c"
    run_expected "control-$control" FAIL
done

# Reconstruct the former signed-shift defect from the current pinned full TU by
# changing one asserted expression. A normal negative slot alias reaches it.
build_objects reconstructed-signed-shift-UBSan "$CLANG" UBSan \
    "$OUT/control-reconstructed-signed-slot-shift.c"
set +e
"$OUT/reconstructed-signed-shift-UBSan.test" "$OUT/retail-function.bin" \
    "$OUT/retail-table-u.bin" "$OUT/retail-table-v.bin" \
    >"$OUT/reconstructed-signed-shift-UBSan.log" 2>&1
old_rc=$?
set -e
if [[ $old_rc -eq 0 ]] || ! rg -q 'runtime error:.*left shift of negative value' \
        "$OUT/reconstructed-signed-shift-UBSan.log"; then
    cat "$OUT/reconstructed-signed-shift-UBSan.build.log" \
        "$OUT/reconstructed-signed-shift-UBSan.log" >&2
    echo "MEMBER_CHANGE_NAME_RETAIL_FAIL reconstructed signed-shift UBSan gate rc=$old_rc" >&2
    exit 1
fi
echo "MEMBER_CHANGE_NAME_RETAIL_RECONSTRUCTED_SIGNED_SHIFT_UBSAN_REJECTED rc=$old_rc"

python3 - "$SOURCE" "$OUT" <<'PY'
from hashlib import sha256
from pathlib import Path
import json, os, subprocess, sys

source = Path(sys.argv[1])
out = Path(sys.argv[2])
pins = json.loads((out / 'pins.json').read_text())
checks = {
    source: pins['source_sha256'],
    Path('disc/member_change_menu.bin'): pins['module_sha256'],
    Path('pc_port/tests/member_change_name_retail_test.c'): pins['test_sha256'],
    Path('pc_port/tests/run_member_change_name_retail_test.sh'): pins['runner_sha256'],
    Path('pc_port/src/battle_mips_adapter.c'): pins['adapter_sha256'],
    Path('pc_port/src/battle_mips_adapter.h'): pins['adapter_header_sha256'],
    Path('pc_port/src/data_member_change_menu.c'): pins['data_member_change_menu_sha256'],
    Path('include/system/menu.h'): pins['menu_header_sha256'],
    Path('include/main/game.h'): pins['game_header_sha256'],
    Path('pc_port/extern/PsyCross/include/PsyX/PsyX_config.h'): pins['psyx_config_sha256'],
    Path('pc_port/build_port.sh'): pins['build_port_sha256'],
}
for path, wanted in checks.items():
    actual = sha256(path.read_bytes()).hexdigest()
    if actual != wanted:
        raise SystemExit(f'MEMBER_CHANGE_NAME_RETAIL_FAIL changed during run {path}: {actual} != {wanted}')
pins['compiler_manifest'] = {
    compiler: subprocess.check_output([compiler, '--version'], text=True).splitlines()[0]
    for compiler in ('gcc', os.environ.get('CLANG', 'clang'))
}
pins['results'] = [
    'production-O0 PASS', 'production-O2 PASS', 'production-UBSan PASS',
    '4 semantic controls rejected',
    'one-site reconstructed signed-shift control rejected by UBSan']
(out / 'manifest.json').write_text(json.dumps(pins, indent=2) + '\n')
PY

echo "MEMBER_CHANGE_NAME_RETAIL_GREEN O0/O2/ClangUBSan 44 cases; 4 controls; reconstructed signed-shift UBSan rejected; evidence=$OUT"
