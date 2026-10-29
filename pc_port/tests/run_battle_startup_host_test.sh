#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ubsan_preflight clang
ulimit -c 0
OUT=${BATTLE_STARTUP_TEST_OUT:-$(mktemp -d /tmp/xeno-battle-startup.XXXXXXXX)}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
printf 'BATTLE STARTUP artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out = pathlib.Path(sys.argv[1])
image = pathlib.Path('disc/battle.bin').read_bytes()
assert hashlib.sha256(image).hexdigest() == '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'
body = image[0x485a8:0x486cc]
table = image[0xf20:0xf38]
display = image[0x48794:0x48864]
assert hashlib.sha256(body).hexdigest() == 'fabcd91ad2aa55111fc92a54dfb9107f2e767ab8d1bf6337bc86baeed23cf186'
assert hashlib.sha256(table).hexdigest() == '997503617c965694684c7402e456d24dac32a33fd1ff4c0c7e116181f16102a2'
assert hashlib.sha256(display).hexdigest() == '9a148103ca4144232c8f32b05b241da03729824b4f7552d1e0467c12f6009184'
paths = ['pc_port/src/battle_module_stage3_host.inc', 'pc_port/src/battle_startup_host.inc', 'pc_port/src/battle_entry_host.inc', 'pc_port/src/battle_display_host.inc',
         'pc_port/src/battle_transition_host.inc',
         'pc_port/src/battle_shard_setup_host.inc', 'pc_port/src/battle_module_setup_host.inc', 'pc_port/src/battle_texture_host.inc', 'pc_port/src/battle_shard_callbacks_host.inc', 'pc_port/src/battle_module_stage1_host.inc', 'pc_port/src/battle_party_stats_host.inc','pc_port/src/battle_module_stage2_host.inc','pc_port/src/battle_turn_speed_host.inc','pc_port/src/battle_target_list_host.inc',
         'pc_port/src/battle_mips_runtime.c', 'pc_port/src/battle_mips_adapter.c',
         'pc_port/tests/battle_startup_host_test.c', 'pc_port/tests/run_battle_startup_host_test.sh',
         'pc_port/extern/PsyCross/src/psx/LIBGPU.C', 'pc_port/extern/PsyCross/src/psx/LIBGTE.C']
(out / 'provenance.json').write_text(json.dumps({
    'scope': 'Retail startup/display vs native bodies separately and composed with modeled callees; separate display integration through actual native SDK initializers; not playable battle acceptance.',
    'code_sha256': hashlib.sha256(body).hexdigest(),
    'table_sha256': hashlib.sha256(table).hexdigest(),
    'display_sha256': hashlib.sha256(display).hexdigest(),
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
sdk_common=(-std=c++17 -fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0
    -ffunction-sections -fdata-sections -fpermissive -w -include pc_port/src/port_compat.h
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
read -r -a sdl_flags <<< "$(pkg-config --cflags sdl2)"
for mode in O0 O2 UBSan; do
    compiler=gcc
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then
        compiler=clang
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/battle_startup_host_test.c -o "$OUT/$mode.test.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/controller_vblank_service.c -o "$OUT/$mode.vblank.o"
    for unit in LIBGPU LIBGTE; do
        g++ "${sdk_common[@]}" "${sdl_flags[@]}" "${flags[@]}" \
            -c "pc_port/extern/PsyCross/src/psx/$unit.C" -o "$OUT/$mode.$unit.o"
    done
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" \
        "$OUT/$mode.LIBGPU.o" "$OUT/$mode.LIBGTE.o" -ldl -lcrypto -o "$OUT/$mode.test"
    if ! "$OUT/$mode.test" > "$OUT/$mode.log" 2>&1; then
        cat "$OUT/$mode.log" >&2
        exit 1
    fi
    cat "$OUT/$mode.log"
    if ! "$OUT/$mode.test" --sdk > "$OUT/$mode.sdk.log" 2>&1; then
        cat "$OUT/$mode.sdk.log" >&2
        exit 1
    fi
    cat "$OUT/$mode.sdk.log"
done
python3 - "$OUT" <<'PY'
import pathlib, sys
out = pathlib.Path(sys.argv[1])
runtime = pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
mutations = {
    'wrong-mode': ('startup', 'case 4:', 'case 5:'),
    'wrong-sixth-argument': ('startup', '0x800ccb94u, 0x800ccbb4u, BS_S0);', '0x800ccb94u, 0x800ccbb4u, BS_S0 + 1u);'),
    'wrong-status-byte': ('startup', 'BS_W8(0x800c4a38u, result);', 'BS_W8(0x800c4a38u, result >> 8);'),
    'unchecked-table': ('startup', 'region < 2', 'region < 1'),
    'wrong-draw-buffer': ('display', 'BD_S0 - 0x5cu', 'BD_S0 - 0x60u'),
    'wrong-fifth-argument': ('display', 'BD_S1 = 224;', 'BD_S1 = 223;'),
    'wrong-screen-height': ('display', 'BD_W16(BD_S0 + 0x0bdau, 216);', 'BD_W16(BD_S0 + 0x0bdau, 224);'),
    'lost-saved-register': ('display', 'BD_S1 = BD_R32(sp + 0x1cu);', '(void)BD_R32(sp + 0x1cu);'),
    'wrong-display-return': ('display', 'entry->cpu->gpr[2] = 216;', 'entry->cpu->gpr[2] = 224;'),
}
for name, (unit, old, new) in mutations.items():
    filename = 'battle_' + unit + '_host.inc'
    body = pathlib.Path('pc_port/src', filename).read_text()
    assert body.count(old) == 1, (name, body.count(old))
    path = out / (name + '.inc')
    path.write_text(body.replace(old, new))
    anchor = '#include "' + filename + '"'
    assert runtime.count(anchor) == 1
    (out / (name + '.c')).write_text(runtime.replace(anchor, '#include "' + str(path) + '"'))
PY
for mutant in wrong-mode wrong-sixth-argument wrong-status-byte unchecked-table \
              wrong-draw-buffer wrong-fifth-argument wrong-screen-height lost-saved-register wrong-display-return; do
    gcc "${common[@]}" -O2 -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
        -c pc_port/tests/battle_startup_host_test.c -o "$OUT/$mutant.o"
    clang -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.vblank.o" "$OUT/O2.LIBGPU.o" "$OUT/O2.LIBGTE.o" \
        -ldl -lcrypto -o "$OUT/$mutant.test"
    status=0
    "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" != 1 ] || ! rg -q '^BATTLE STARTUP FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        printf 'BATTLE STARTUP control failed: %s status=%s\n' "$mutant" "$status" >&2
        exit 1
    fi
    printf 'BATTLE STARTUP negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out = pathlib.Path(sys.argv[1])
for path, digest in json.loads((out / 'provenance.json').read_text())['source_pins'].items():
    assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest() == digest, path
print('BATTLE STARTUP source pins unchanged')
PY
