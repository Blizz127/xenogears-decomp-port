#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ubsan_preflight clang
ulimit -c 0
OUT=${BATTLE_TRANSITION_TEST_OUT:-$(mktemp -d /tmp/xeno-battle-transition.XXXXXXXX)}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
printf 'BATTLE TRANSITION artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out = pathlib.Path(sys.argv[1])
image = pathlib.Path('disc/battle.bin').read_bytes()
assert hashlib.sha256(image).hexdigest() == '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'
body = image[0x47d80:0x48138]
table = image[0xf0c:0xf20]
assert hashlib.sha256(body).hexdigest() == '98b082a1b8b09298d58fcdf513a73da26b26cc4e46a44e7ca159e541693e1321'
assert hashlib.sha256(table).hexdigest() == 'f8d2b85319a5ddcb5e31ce1ac671365b3b728957de9112f62e479a3a3f500f0d'
paths = ['pc_port/src/battle_module_stage3_host.inc', 'pc_port/src/battle_transition_host.inc', 'pc_port/src/battle_entry_host.inc',
         'pc_port/src/battle_shard_setup_host.inc', 'pc_port/src/battle_module_setup_host.inc', 'pc_port/src/battle_texture_host.inc', 'pc_port/src/battle_shard_callbacks_host.inc', 'pc_port/src/battle_module_stage1_host.inc', 'pc_port/src/battle_party_stats_host.inc','pc_port/src/battle_module_stage2_host.inc','pc_port/src/battle_turn_speed_host.inc','pc_port/src/battle_target_list_host.inc',
         'pc_port/src/battle_startup_host.inc', 'pc_port/src/battle_display_host.inc',
         'pc_port/src/battle_mips_runtime.c', 'pc_port/src/battle_mips_adapter.c',
         'pc_port/tests/battle_transition_host_test.c', 'pc_port/tests/run_battle_transition_host_test.sh']
(out / 'provenance.json').write_text(json.dumps({
    'scope': 'Retail transition vs native body; modeled callee boundaries, not visual/audio/timing or playable battle acceptance.',
    'code_sha256': hashlib.sha256(body).hexdigest(),
    'table_sha256': hashlib.sha256(table).hexdigest(),
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
    clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/battle_transition_host_test.c -o "$OUT/$mode.test.o"
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
runtime = pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body = pathlib.Path('pc_port/src/battle_transition_host.inc').read_text()
mutations = {
    'short-pixel-loop': ('i < 0x14000u;', 'i < 0x11800u;'),
    'short-wait': ('BT_S3 = 0x56;', 'BT_S3 = 0x55;'),
    'late-loader-state': ('BT_S2++;\n                BT_CALL0(0x8001bb0cu, 0x800b7a44u);',
                          'BT_CALL0(0x8001bb0cu, 0x800b7a44u);\n                BT_S2++;'),
    'wrong-scratch-stack': ('entry->cpu->gpr[29] = 0x1f8003f8u;', 'entry->cpu->gpr[29] = 0x1f8003f4u;'),
    'wrong-stack-restore': ('BT_R32(entry->cpu->gpr[29] + 4u)', 'BT_R32(entry->cpu->gpr[29] + 8u)'),
    'wrong-color-buffer': ('BT_W8(BT_S1 + BT_R32(BT_S0 + 0xc84u) * 0x4070u + offset, color);',
                           'BT_W8(BT_S1 + (BT_R32(BT_S0 + 0xc84u) ^ 1u) * 0x4070u + offset, color);'),
    'wrong-vsync': ('BT_CALL(0x8004b54cu, 0x800b7aa4u, 2);', 'BT_CALL(0x8004b54cu, 0x800b7aa4u, 1);'),
    'lost-saved-register': ('BT_S5 = BT_R32(sp + 0x2cu);', '(void)BT_R32(sp + 0x2cu);'),
    'unchecked-table': ('region < 2', 'region < 1'),
}
for name, (old, new) in mutations.items():
    assert body.count(old) == 1, (name, body.count(old))
    path = out / (name + '.inc')
    path.write_text(body.replace(old, new))
    anchor = '#include "battle_transition_host.inc"'
    assert runtime.count(anchor) == 1
    (out / (name + '.c')).write_text(runtime.replace(anchor, '#include "' + str(path) + '"'))
PY
for mutant in short-pixel-loop short-wait late-loader-state wrong-scratch-stack \
              wrong-stack-restore wrong-color-buffer wrong-vsync lost-saved-register unchecked-table; do
    gcc "${common[@]}" -O2 -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
        -c pc_port/tests/battle_transition_host_test.c -o "$OUT/$mutant.o"
    clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.vblank.o" -ldl -lcrypto -o "$OUT/$mutant.test"
    status=0
    "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" != 1 ] || ! rg -q '^BATTLE TRANSITION FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        printf 'BATTLE TRANSITION control failed: %s status=%s\n' "$mutant" "$status" >&2
        exit 1
    fi
    printf 'BATTLE TRANSITION negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out = pathlib.Path(sys.argv[1])
for path, digest in json.loads((out / 'provenance.json').read_text())['source_pins'].items():
    assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest() == digest, path
print('BATTLE TRANSITION source pins unchanged')
PY
