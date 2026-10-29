#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
OUT=${BATTLE_MODULE_TEST_OUT:-$(mktemp -d /tmp/xeno-battle-module.XXXXXXXX)}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE MODULE SETUP artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PYCODE'
import hashlib, io, json, pathlib, sys
sys.path.insert(0, 'tools/scripts')
from cdrom.cdxa import CdromXa
out = pathlib.Path(sys.argv[1])
with open('disc/disc1.bin', 'rb') as image:
    disc_hash = hashlib.file_digest(image, 'sha256').hexdigest()
assert disc_hash == '39c547a9afc6da15d847ef81a2c6cea1a6516bdfa562cf13b0999b04e8598bda'
# Archive 0x0c/file 4 in this pinned US disc: 22208 raw bytes at sector 239777.
buffer = io.BytesIO()
with CdromXa('disc/disc1.bin') as image:
    image.extract_sectors(239777, 22208, buffer)
module = buffer.getvalue()[:22208]
assert hashlib.sha256(module).hexdigest() == '4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885'
assert hashlib.sha256(module[0x1384:0x1840]).hexdigest() == 'e7a0cdba89318569f55dedcac5134df520eab4e352b4fe6495c7741dccb646a5'
assert hashlib.sha256(module[0x1840:0x1924]).hexdigest() == 'e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2'
(out / 'module.bin').write_bytes(module)
paths = ['pc_port/src/battle_module_stage3_host.inc', 'pc_port/src/battle_module_setup_host.inc', 'pc_port/src/battle_texture_host.inc', 'pc_port/src/battle_shard_callbacks_host.inc', 'pc_port/src/battle_module_stage1_host.inc', 'pc_port/src/battle_party_stats_host.inc','pc_port/src/battle_module_stage2_host.inc','pc_port/src/battle_turn_speed_host.inc','pc_port/src/battle_target_list_host.inc', 'pc_port/src/battle_entry_host.inc',
         'pc_port/src/battle_shard_setup_host.inc', 'pc_port/src/battle_transition_host.inc',
         'pc_port/src/battle_startup_host.inc', 'pc_port/src/battle_display_host.inc',
         'pc_port/src/battle_mips_runtime.c', 'pc_port/src/battle_mips_adapter.c',
         'pc_port/src/psyq_compat.c', 'pc_port/src/controller_vblank_service.c',
         'pc_port/tests/battle_module_setup_host_test.c', 'pc_port/tests/run_battle_module_setup_host_test.sh']
(out / 'provenance.json').write_text(json.dumps({
    'scope': 'Retail module callers vs native bodies with modeled callees; not actual loader/decompression/SDK or playable transition acceptance.',
    'disc_sha256': disc_hash, 'module_sha256': hashlib.sha256(module).hexdigest(),
    'source_pins': {p: hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in paths}
}, indent=2) + '\n')
PYCODE
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
        -c pc_port/tests/battle_module_setup_host_test.c -o "$OUT/$mode.test.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/controller_vblank_service.c -o "$OUT/$mode.vblank.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" -ldl -lcrypto -o "$OUT/$mode.test"
    if ! "$OUT/$mode.test" "$OUT/module.bin" > "$OUT/$mode.log" 2>&1; then
        cat "$OUT/$mode.log" >&2
        exit 1
    fi
    cat "$OUT/$mode.log"
done
python3 - "$OUT" <<'PYCODE'
import pathlib, sys
out = pathlib.Path(sys.argv[1])
runtime = pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body = pathlib.Path('pc_port/src/battle_module_setup_host.inc').read_text()
mutations = {
    'unmasked-stage': ('entry->cpu->gpr[4] &= 0xffu;', 'entry->cpu->gpr[4] &= 0xffffffffu;'),
    'wrong-stage-order': ('BM_CALL0(0x801e4870u, 0x801e58acu);', 'BM_CALL0(0x801e4ac0u, 0x801e58acu);'),
    'wrong-default-return': ('stage < 2u ? 1u : 3u', 'stage < 2u ? 1u : 0u'),
    'unmasked-selection': ('(selected & 0xffffu) != 0', 'selected != 0'),
    'wrong-party-stride': ('BM_S2 += 0x170u;', 'BM_S2 += 0x16cu;'),
    'wrong-gear-sentinel': ('if (gear == 0xffu) gear = 0;', 'if (gear == 0xfeu) gear = 0;'),
    'wrong-seventh-argument': ('BM_S1, 0, 0, 0, 0, 0, 0);', 'BM_S1, 0, 0, 0, 0, 0, 1);'),
    'lost-fp': ('BM_FP = BM_R32(sp + 0x40u);', '(void)BM_R32(sp + 0x40u);'),
    'wrong-file-id': ('scenario * 2u + 3u);', 'scenario * 2u + 4u);'),
    'one-region-gate': ('region < 2', 'region < 1'),
}
for name, (old, new) in mutations.items():
    expected_count = 2 if name == 'wrong-file-id' else 1
    assert body.count(old) == expected_count, (name, body.count(old))
    path = out / (name + '.inc')
    path.write_text(body.replace(old, new, 1))
    anchor = '#include "battle_module_setup_host.inc"'
    assert runtime.count(anchor) == 1
    (out / (name + '.c')).write_text(runtime.replace(anchor, '#include "' + str(path) + '"'))
PYCODE
for mutant in unmasked-stage wrong-stage-order wrong-default-return unmasked-selection \
              wrong-party-stride wrong-gear-sentinel wrong-seventh-argument lost-fp wrong-file-id one-region-gate; do
    gcc "${common[@]}" -O2 -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
        -c pc_port/tests/battle_module_setup_host_test.c -o "$OUT/$mutant.o"
    clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.vblank.o" -ldl -lcrypto -o "$OUT/$mutant.test"
    status=0
    "$OUT/$mutant.test" "$OUT/module.bin" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" != 1 ] || ! rg -q '^BATTLE MODULE SETUP FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        printf 'BATTLE MODULE SETUP control failed: %s status=%s\n' "$mutant" "$status" >&2
        exit 1
    fi
    printf 'BATTLE MODULE SETUP negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PYCODE'
import hashlib, json, pathlib, sys
out = pathlib.Path(sys.argv[1])
for path, digest in json.loads((out / 'provenance.json').read_text())['source_pins'].items():
    assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest() == digest, path
print('BATTLE MODULE SETUP source pins unchanged')
PYCODE
