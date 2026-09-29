#!/usr/bin/env bash
# battle_load_image_cache — regression certificate for two fixes in
# pc_port/src/battle_mips_runtime.c:
#   1. LoadImage guest-pointer translation (low physical-RAM source and KSEG0
#      RECT), which previously SIGSEGVd in PsyCross's GR_CopyVRAM.
#   2. The resolved-call cache that stopped every unresolved guest call from
#      re-running a func_%08X dlsym (the battle stall).
#
# UBSan: this host's gcc has no runtime, so the runner probes gcc and falls back
# to clang, printing which ran; it never silently skips.
#
# Artifacts go under $TMPDIR, never pc_port/build_native/.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
SOURCE=${LOADIMAGE_CACHE_SOURCE:-"$PWD/pc_port/src/battle_mips_runtime.c"}
SOURCE=$(realpath "$SOURCE")
OUT=${LOADIMAGE_CACHE_OUT:-$(mktemp -d /tmp/xeno-loadimage-cache-test.XXXXXXXX)}
mkdir -p "$OUT"
printf 'LOADIMAGE CACHE source=%s artifacts=%s\n' "$SOURCE" "$OUT"

# Provenance: the retail call site and its address are pinned.
python3 - "$OUT" "$SOURCE" <<'PY'
from pathlib import Path
from hashlib import sha256
import json, re, sys
out = Path(sys.argv[1]); source = Path(sys.argv[2])
funcs = Path('linker/undefined_funcs_auto.battle.txt').read_text()
assert re.search(r'^LoadImage\s*=\s*0x80044894\s*;', funcs, re.M), \
    'LoadImage no longer maps to retail 0x80044894'
asm = Path('asm/slus_006.64/matchings/psyq/libgpu/LoadImage.s').read_text()
assert 'glabel LoadImage' in asm, 'retail LoadImage assembly label missing'
src = source.read_text()
assert 'bridge_load_image' in src and '"LoadImage"' in src, \
    'production no longer dispatches LoadImage through its own bridge'
assert 'BridgeCallCacheSlot' in src and 'bridge_cache_slot' in src, \
    'production no longer carries the resolved-call cache'
paths = [source, Path('pc_port/tests/battle_load_image_cache_test.c'),
         Path('pc_port/tests/run_battle_load_image_cache_test.sh')]
(out / 'provenance.json').write_text(json.dumps({
    'scope': 'battle bridge LoadImage pointer translation + resolved-call cache',
    'retail_loadimage_address': '0x80044894',
    'source_pins': {str(p.resolve()): sha256(p.read_bytes()).hexdigest() for p in paths},
}, indent=2) + '\n')
print('LOADIMAGE CACHE provenance: LoadImage=0x80044894 -> bridge + cache present PASS')
PY

bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"

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

TEST_SRC=pc_port/tests/battle_load_image_cache_test.c
COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

build_and_run() {
    local mode="$1" cc="$2" src="$3"; shift 3
    local flags=("$@")
    "$cc" "${COMMON[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
    "$cc" "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$src\"" -c "$TEST_SRC" -o "$OUT/$mode.test.o"
    "$cc" "${COMMON[@]}" "${flags[@]}" -w -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    "$cc" "${COMMON[@]}" "${flags[@]}" -w -c pc_port/src/controller_vblank_service.c -o "$OUT/$mode.vblank.o"
    "$cc" "${COMMON[@]}" "${flags[@]}" -w -c "$OUT/stub_override.c" -o "$OUT/$mode.stub.o"
    "$cc" -no-pie "${flags[@]}" -Wl,--gc-sections \
        "$OUT/$mode.vblank.o" "$OUT/$mode.compat.o" "$OUT/$mode.test.o" \
        "$OUT/$mode.cpu.o" "$OUT/$mode.stub.o" -ldl -o "$OUT/$mode"
    local status=0
    "$OUT/$mode" > "$OUT/$mode.log" 2>&1 || status=$?
    cat "$OUT/$mode.log"
    return "$status"
}

echo "== production regimes =="
for mode in O0 O2; do
    if ! build_and_run "$mode" gcc "$SOURCE" -"$mode"; then
        printf 'LOADIMAGE CACHE REGIME FAILURE %s\n' "$mode" >&2; exit 1
    fi
    grep -q '^LOADIMAGE CACHE PASS ' "$OUT/$mode.log" || { echo "LOADIMAGE CACHE REGIME FAILURE $mode PASS line missing" >&2; exit 1; }
    printf '  %s (gcc): PASS\n' "$mode"
done

cat > "$OUT/ubsan_probe.c" <<'EOF'
int main(void) { return 0; }
EOF
ubsan_cc=""
if gcc -std=gnu17 -O1 -fsanitize=undefined -fno-sanitize-recover=all "$OUT/ubsan_probe.c" -o "$OUT/ubsan_probe.gcc" >/dev/null 2>&1; then
    ubsan_cc=gcc
    printf 'UBSan regime: gcc (probe linked)\n'
elif clang -std=gnu17 -O1 -fsanitize=undefined -fno-sanitize-recover=all "$OUT/ubsan_probe.c" -o "$OUT/ubsan_probe.clang" >/dev/null 2>&1; then
    ubsan_cc=clang
    printf 'UBSan regime: clang (gcc runtime probe failed)\n'
else
    echo 'LOADIMAGE CACHE UBSAN UNAVAILABLE: neither gcc nor clang can build -fsanitize=undefined' >&2
    exit 1
fi
if ! build_and_run UBSan "$ubsan_cc" "$SOURCE" -O1 -fsanitize=undefined -fno-sanitize-recover=all; then
    echo "LOADIMAGE CACHE REGIME FAILURE UBSan" >&2; exit 1
fi
grep -q '^LOADIMAGE CACHE PASS ' "$OUT/UBSan.log" || { echo "LOADIMAGE CACHE REGIME FAILURE UBSan PASS line missing" >&2; exit 1; }
printf '  UBSan (%s): PASS\n' "$ubsan_cc"

# ---------------------------------------------------------------------------
# Mutants: whole-file edits of the production source.  Each must make the test
# fail a named assertion (never the -Werror build).
# ---------------------------------------------------------------------------
python3 - "$OUT" "$SOURCE" <<'PY'
from pathlib import Path
import json, sys
out = Path(sys.argv[1]); source = Path(sys.argv[2]).read_text()
changes = {
    # Low physical source pointer stops being mapped into g_PsxRam.
    'loadimage-no-low-map': (
        '    if (source < 0x200000u)\n        data = g_PsxRam + source;\n    else\n        data = (void *)translate_argument(runtime, source);',
        '    data = (void *)translate_argument(runtime, source);'),
    # Low RECT pointer stops getting the KSEG0 bit.
    'loadimage-no-rect-kseg0': (
        '    if (address < 0x200000u)\n        address |= 0x80000000u;',
        '    (void)0;'),
    # The cache never stores its key, so it can never hit.
    'cache-key-dropped': (
        '        slot->target = target;\n        slot->state =',
        '        slot->state ='),
    # Cached entries are ignored and every call re-resolves.
    'cache-ignored': (
        '    } else if (slot->state == 1) {\n        resolved = &slot->entry;',
        '    } else if (0) {\n        resolved = &slot->entry;'),
    # Eviction returns a live slot, so another target's verdict is used as a hit.
    'cache-evict-stale': (
        '        BridgeCallCacheSlot *victim = &runtime->call_cache[base];\n        victim->state = 0;\n        return victim;',
        '        return &runtime->call_cache[base];'),
    # LoadImage stops resolving the RECT through guest memory.
    'loadimage-rect-not-guest': (
        '    rect = resolve_memory(runtime, address, 8u, 0);',
        '    rect = (void *)(uintptr_t)address;'),
}
manifest = {}
for name, (old, new) in changes.items():
    assert source.count(old) == 1, (name, source.count(old))
    code = source.replace(old, new, 1)
    assert code != source, name
    (out / (name + '.c')).write_text(code)
    manifest[name] = name
(out / 'negative-controls.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('LOADIMAGE CACHE mutants written:', ' '.join(manifest))
PY

echo "== mutants (each must be rejected) =="
MUTANTS=(loadimage-no-low-map loadimage-no-rect-kseg0 cache-key-dropped cache-ignored cache-evict-stale loadimage-rect-not-guest)
for mutant in "${MUTANTS[@]}"; do
    set +e
    # Not "$OUT/$mutant.log": build_and_run writes and cats that file itself
    # (GNU cat refuses "input file is output file" and the FAIL line is lost).
    build_and_run "$mutant" gcc "$OUT/$mutant.c" -O0 -w > "$OUT/$mutant.run.log" 2>&1
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! grep -q '^LOADIMAGE CACHE FAIL ' "$OUT/$mutant.run.log"; then
        echo "LOADIMAGE CACHE MUTANT NOT DETECTED: $mutant rc=$rc" >&2
        cat "$OUT/$mutant.run.log" >&2
        exit 1
    fi
    printf 'LOADIMAGE CACHE mutant rejected: %s\n' "$mutant"
done

python3 - "$SOURCE" <<'PY'
from pathlib import Path
from hashlib import sha256
import sys
print('LOADIMAGE CACHE source pin unchanged:', sha256(Path(sys.argv[1]).read_bytes()).hexdigest()[:16])
PY
echo 'LOADIMAGE CACHE PASS (3 regimes, 6/6 mutants rejected)'
