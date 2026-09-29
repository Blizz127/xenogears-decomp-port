#!/usr/bin/env bash
# battle_main_exe_callback_dispatch — regression certificate for the
# main-executable branch of PcPort_BattleMipsDispatchCallback.
#
# The interpreted battle overlay's func_800B6438 (src/battle/mainc88.c) packs
# the retail main-executable address 0x80025A88 (D_80025A88) into a work-list
# callback slot. pc_port/src/work_list_port.c hands that callback to
# PcPort_BattleMipsDispatchCallback; before the fix that function returned 0
# for every callback outside the battle-overlay guest range, so the work list
# aborted with "unresolved guest callback 0x80025a88". This runner pins the
# retail call site and then builds pc_port/tests/
# battle_main_exe_callback_dispatch_test.c at O0, O2 and UBSan.
#
# UBSan: this host's gcc has no runtime (missing /usr/lib64/libubsan.so.1.0.0),
# so the runner probes gcc, falls back to clang, and prints which compiler ran
# the UBSan regime. A compiler that cannot build UBSan is never silently
# skipped: if neither gcc nor clang can, the runner fails.
#
# Artifacts go under $TMPDIR, never pc_port/build_native/, so a concurrent
# build_port.sh cannot race with this test.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
SOURCE=${MAIN_EXE_CALLBACK_SOURCE:-"$PWD/pc_port/src/battle_mips_runtime.c"}
SOURCE=$(realpath "$SOURCE")
OUT=${MAIN_EXE_CALLBACK_OUT:-$(mktemp -d /tmp/xeno-main-exe-callback-test.XXXXXXXX)}
mkdir -p "$OUT"
printf 'MAIN EXE CALLBACK source=%s artifacts=%s\n' "$SOURCE" "$OUT"

# ---------------------------------------------------------------------------
# Provenance: the crash call site and the address under test are retail-pinned.
# ---------------------------------------------------------------------------
python3 - "$OUT" "$SOURCE" <<'PY'
from pathlib import Path
from hashlib import sha256
import json, re, sys
out = Path(sys.argv[1]); source = Path(sys.argv[2])

mainc88 = Path('src/battle/mainc88.c').read_text()
assert 'WorkListSetTaskCallback' in mainc88, 'mainc88.c callback store missing'
assert re.search(r'WorkListSetTaskCallback\([^;]*D_80025A88\s*\)', mainc88), \
    'func_800B6438 no longer stores D_80025A88 in a packed callback slot'

syms = Path('linker/undefined_syms_auto.battle.txt').read_text()
m = re.search(r'^D_80025A88\s*=\s*0x80025A88\s*;\s*$', syms, re.M)
assert m, 'D_80025A88 no longer maps to retail 0x80025A88'

worklist = Path('pc_port/src/work_list_port.c').read_text()
assert 'PcPort_BattleMipsDispatchCallback(address, argument)' in worklist, \
    'work_list_port.c no longer dispatches the packed callback'
assert 'unresolved guest callback' in worklist, \
    'work_list_port.c no longer reports the unresolved-callback abort'

paths = [source, Path('pc_port/src/work_list_port.c'),
         Path('src/battle/mainc88.c'),
         Path('pc_port/tests/battle_main_exe_callback_dispatch_test.c'),
         Path('pc_port/tests/run_battle_main_exe_callback_dispatch_test.sh')]
(out / 'provenance.json').write_text(json.dumps({
    'scope': 'main-executable packed work-list callback dispatch, retail D_80025A88',
    'retail_callback_address': '0x80025A88',
    'source_pins': {str(p.resolve()): sha256(p.read_bytes()).hexdigest() for p in paths},
}, indent=2) + '\n')
print('MAIN EXE CALLBACK provenance: mainc88.c -> D_80025A88=0x80025A88 -> work_list_port dispatch PASS')
PY

# ---------------------------------------------------------------------------
# Bridge map (battle_mips_runtime.c includes battle_bridge_map.inc).
# ---------------------------------------------------------------------------
bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"

# ---------------------------------------------------------------------------
# Strong xeno_port_is_generated_stub override.  The production TU carries a
# weak definition, and a single TU cannot define the symbol twice, so the
# strong definition lives here and consults tables owned by the test.
# ---------------------------------------------------------------------------
cat > "$OUT/stub_override.c" <<'EOF'
#include <string.h>
extern const char *g_test_generated_stub_names[];
extern unsigned g_test_generated_stub_count;
int xeno_port_is_generated_stub(const char *name)
{
    unsigned i;
    for (i = 0; i < g_test_generated_stub_count; i++)
        if (strcmp(name, g_test_generated_stub_names[i]) == 0)
            return 1;
    return 0;
}
EOF

TEST_SRC=pc_port/tests/battle_main_exe_callback_dispatch_test.c
COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
# -rdynamic would export every psyq_compat.c symbol and defeat --gc-sections
# (whose undefined references this minimal link does not satisfy), so export
# only the func_%08X fallback owner the test's dlsym lookup needs.
EXPORT=(-Wl,--export-dynamic-symbol=func_80025A88)

# ---------------------------------------------------------------------------
# One regime: compile every object with one compiler/flags, link, run.
# ---------------------------------------------------------------------------
build_and_run() {
    local mode="$1" cc="$2"; shift 2
    local flags=("$@")
    "$cc" "${COMMON[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c \
        -o "$OUT/$mode.compat.o"
    "$cc" "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$SOURCE\"" -c "$TEST_SRC" -o "$OUT/$mode.test.o"
    "$cc" "${COMMON[@]}" "${flags[@]}" -w -c pc_port/src/battle_mips_adapter.c \
        -o "$OUT/$mode.cpu.o"
    "$cc" "${COMMON[@]}" "${flags[@]}" -w -c pc_port/src/controller_vblank_service.c \
        -o "$OUT/$mode.vblank.o"
    "$cc" "${COMMON[@]}" "${flags[@]}" -w -c "$OUT/stub_override.c" \
        -o "$OUT/$mode.stub.o"
    "$cc" -no-pie "${flags[@]}" -Wl,--gc-sections "${EXPORT[@]}" \
        "$OUT/$mode.vblank.o" "$OUT/$mode.compat.o" "$OUT/$mode.test.o" \
        "$OUT/$mode.cpu.o" "$OUT/$mode.stub.o" -ldl -o "$OUT/$mode"
    local status=0
    "$OUT/$mode" > "$OUT/$mode.log" 2>&1 || status=$?
    cat "$OUT/$mode.log"
    if [ "$status" -ne 0 ] || ! grep -q '^MAIN EXE CALLBACK PASS ' "$OUT/$mode.log"; then
        printf 'MAIN EXE CALLBACK REGIME FAILURE %s status=%s\n' "$mode" "$status" >&2
        exit 1
    fi
    printf '  %s (%s): PASS\n' "$mode" "$cc"
}

echo "== production regimes =="
build_and_run O0 gcc -O0
build_and_run O2 gcc -O2

# UBSan: probe gcc first, fall back to clang, print the winner.  Never skip.
cat > "$OUT/ubsan_probe.c" <<'EOF'
int main(void) { return 0; }
EOF
ubsan_cc=""
if gcc -std=gnu17 -O1 -fsanitize=undefined -fno-sanitize-recover=all \
        "$OUT/ubsan_probe.c" -o "$OUT/ubsan_probe.gcc" >/dev/null 2>&1; then
    ubsan_cc=gcc
    printf 'UBSan regime: gcc (gcc -fsanitize=undefined probe linked)\n'
elif clang -std=gnu17 -O1 -fsanitize=undefined -fno-sanitize-recover=all \
        "$OUT/ubsan_probe.c" -o "$OUT/ubsan_probe.clang" >/dev/null 2>&1; then
    ubsan_cc=clang
    printf 'UBSan regime: clang (gcc runtime probe failed; clang -fsanitize=undefined linked)\n'
else
    echo "MAIN EXE CALLBACK FATAL: neither gcc nor clang can build -fsanitize=undefined (never skipping UBSan)" >&2
    exit 1
fi
build_and_run UBSan "$ubsan_cc" -O1 -fsanitize=undefined -fno-sanitize-recover=all

# ---------------------------------------------------------------------------
# Negative controls: each is a source-level mutation of the production file
# applied to a copy, and each MUST make the test fail a named assertion.
# ---------------------------------------------------------------------------
python3 - "$OUT" "$SOURCE" <<'PYCONTROL'
from pathlib import Path
from hashlib import sha256
import json, sys
out = Path(sys.argv[1]); source = Path(sys.argv[2]).read_text()
a = source.index('static int run_main_exe_callback(')
b = source.index('int PcPort_BattleMipsDispatchCallback(', a)
body = source[a:b]
changes = {
    # Old behaviour: callbacks outside the battle-overlay guest range are not ours.
    'old-main-exe-early-return': (
        '{\n    const ResolvedFunction *resolved = find_function(runtime, callback);',
        '{\n    if (!target_is_guest_code(callback)) return 0;\n'
        '    const ResolvedFunction *resolved = find_function(runtime, callback);'),
    # Dispatch the host with the wrong (null) argument.
    'host-wrong-argument': (
        '((void (*)(void *))host)(argument);',
        '((void (*)(void *))host)(NULL);'),
    # Drop the generated-stub refusal on the bridge-table path.
    'drop-bridge-stub-refusal': (
        '    if (resolved != NULL && resolved->host != NULL &&\n'
        '        !xeno_port_is_generated_stub(resolved->name)) {',
        '    if (resolved != NULL && resolved->host != NULL) {'),
    # Drop the generated-stub refusal on the func_%08X dlsym path.
    'drop-dlsym-stub-refusal': (
        'if (host == NULL || xeno_port_is_generated_stub(fallback))',
        'if (host == NULL)'),
    # Report success without ever calling the host.
    'success-without-call': (
        '    ((void (*)(void *))host)(argument);\n    return 1;',
        '    (void)host;\n    (void)argument;\n    return 1;'),
}
manifest = {}
for name, (old, new) in changes.items():
    assert body.count(old) == 1, (name, body.count(old))
    code = source[:a] + body.replace(old, new, 1) + source[b:]
    assert code != source, name
    (out / (name + '.c')).write_text(code)
    manifest[name] = sha256(code.encode()).hexdigest()
(out / 'negative-controls.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('MAIN EXE CALLBACK mutants written:', ' '.join(manifest))
PYCONTROL

echo "== mutants (each must be rejected) =="
MUTANTS=(old-main-exe-early-return host-wrong-argument drop-bridge-stub-refusal
         drop-dlsym-stub-refusal success-without-call)
for mutant in "${MUTANTS[@]}"; do
    # -w: a mutation may intentionally leave a parameter unused; the control
    # must fail a test assertion, never the -Werror build.
    gcc "${COMMON[@]}" -O2 -w \
        "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" -c "$TEST_SRC" \
        -o "$OUT/$mutant.test.o"
    gcc -no-pie -O2 -Wl,--gc-sections "${EXPORT[@]}" \
        "$OUT/O2.vblank.o" "$OUT/O2.compat.o" "$OUT/$mutant.test.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.stub.o" -ldl -o "$OUT/$mutant"
    status=0
    "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" -ne 1 ] || ! grep -q '^MAIN EXE CALLBACK FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        printf 'MAIN EXE CALLBACK CONTROL FAILURE %s status=%s\n' "$mutant" "$status" >&2
        exit 1
    fi
    printf 'negative control detected: %s (rc=%s, %s)\n' \
        "$mutant" "$status" "$(grep -m1 '^MAIN EXE CALLBACK FAIL ' "$OUT/$mutant.log")"
done

# ---------------------------------------------------------------------------
# The production source and the harness must be byte-identical after the run.
# ---------------------------------------------------------------------------
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json, sys
p = json.loads((Path(sys.argv[1]) / 'provenance.json').read_text())
for path, expected in p['source_pins'].items():
    assert sha256(Path(path).read_bytes()).hexdigest() == expected, path
print('MAIN EXE CALLBACK source pins unchanged')
PY

printf 'MAIN EXE CALLBACK PASS (%s/%s negative controls rejected)\n' \
    "${#MUTANTS[@]}" "${#MUTANTS[@]}"
