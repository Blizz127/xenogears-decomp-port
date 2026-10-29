#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ubsan_preflight clang
ulimit -c 0
OUT=${BATTLE_ENTRY_TEST_OUT:-$(mktemp -d /tmp/xeno-battle-entry.XXXXXXXX)}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
printf 'BATTLE ENTRY artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out = pathlib.Path(sys.argv[1])
image = pathlib.Path('disc/battle.bin').read_bytes()
assert hashlib.sha256(image).hexdigest() == '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'
entry = image[0x1450:0x1be8]
assert hashlib.sha256(entry).hexdigest() == '2df67f9cb21d6e025ed9164fcaae41dd971a2fd1c5f1bac0dcd0687aa4e54a02'
paths = ['pc_port/src/battle_module_stage3_host.inc', 'pc_port/src/battle_entry_host.inc', 'pc_port/src/battle_startup_host.inc', 'pc_port/src/battle_display_host.inc',
         'pc_port/src/battle_transition_host.inc', 'pc_port/src/battle_mips_runtime.c',
         'pc_port/src/battle_shard_setup_host.inc', 'pc_port/src/battle_module_setup_host.inc', 'pc_port/src/battle_texture_host.inc', 'pc_port/src/battle_shard_callbacks_host.inc', 'pc_port/src/battle_module_stage1_host.inc', 'pc_port/src/battle_party_stats_host.inc','pc_port/src/battle_module_stage2_host.inc','pc_port/src/battle_turn_speed_host.inc','pc_port/src/battle_target_list_host.inc',
         'pc_port/src/battle_mips_adapter.c', 'pc_port/tests/battle_entry_host_test.c',
         'pc_port/tests/run_battle_entry_host_test.sh']
(out / 'provenance.json').write_text(json.dumps({
    'scope': 'Retail battle entry vs production native entry; modeled callee boundaries; not playable battle acceptance.',
    'entry_sha256': hashlib.sha256(entry).hexdigest(),
    'source_pins': {p: hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in paths}
}, indent=2) + '\n')
PY
python3 tools/scripts/gen_battle_bridge_map.py \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"
common=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    compiler=gcc
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then
        compiler=clang
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    # Existing native GTE helpers satisfy the production bridge's link surface;
    # the entry has no GTE calls (the test aborts if those boundaries are used).
    clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/battle_entry_host_test.c -o "$OUT/$mode.test.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/controller_vblank_service.c -o "$OUT/$mode.vblank.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" -ldl -lcrypto -o "$OUT/$mode.test"
    if ! "$OUT/$mode.test" > "$OUT/$mode.log" 2>&1; then
        cat "$OUT/$mode.log" >&2
        exit 1
    fi
    cat "$OUT/$mode.log"
done
python3 - "$OUT" <<'PY'
import pathlib, sys
out = pathlib.Path(sys.argv[1])
entry = pathlib.Path('pc_port/src/battle_entry_host.inc').read_text()
runtime = pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
mutations = {
    'wrong-reservation': ('BE_R32(0x800d2d3cu) - 0x801de000u', 'BE_R32(0x800d2d3cu) - 0x801e0000u'),
    'wrong-stack-argument': ('callee.gpr[29] + 0x10u + (i - 4u)', 'callee.gpr[29] + 0x14u + (i - 4u)'),
    'lost-entry-s2': ('BE_S3 = 1;\n    BE_W32', 'BE_S3 = 1; BE_S2 = 0;\n    BE_W32'),
    'hidden-callee-fallback': ('entry->cpu->steps = callee.steps;', '(void)callee.steps;'),
}
for name, (old, new) in mutations.items():
    assert entry.count(old) == 1, (name, entry.count(old))
    path = out / (name + '.inc')
    path.write_text(entry.replace(old, new))
    anchor = '#include "battle_entry_host.inc"'
    assert runtime.count(anchor) == 1
    (out / (name + '.c')).write_text(runtime.replace(anchor, '#include "' + str(path) + '"'))
PY
for mutant in wrong-reservation wrong-stack-argument lost-entry-s2 hidden-callee-fallback; do
    gcc "${common[@]}" -O2 -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
        -c pc_port/tests/battle_entry_host_test.c -o "$OUT/$mutant.o"
    clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.vblank.o" -ldl -lcrypto -o "$OUT/$mutant.test"
    status=0
    "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" != 1 ] || ! rg -q '^BATTLE ENTRY FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        printf 'BATTLE ENTRY control failed: %s status=%s\n' "$mutant" "$status" >&2
        exit 1
    fi
    printf 'BATTLE ENTRY negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out = pathlib.Path(sys.argv[1])
for path, digest in json.loads((out / 'provenance.json').read_text())['source_pins'].items():
    assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest() == digest, path
print('BATTLE ENTRY source pins unchanged')
PY
