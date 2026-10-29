#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
OUT=${BATTLE_SHARD_TEST_OUT:-$(mktemp -d /tmp/xeno-battle-shard.XXXXXXXX)}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE SHARD SETUP artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out = pathlib.Path(sys.argv[1])
image = pathlib.Path('disc/battle.bin').read_bytes()
assert hashlib.sha256(image).hexdigest() == '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'
alloc = image[0x478fc:0x47934]
init = image[0x47934:0x47d80]
assert hashlib.sha256(alloc).hexdigest() == '4b4584aa569b7bf0707ee0a59028d0bf82443e6bdd54e2aa9916aa6d15ce3f95'
assert hashlib.sha256(init).hexdigest() == '664c99b631d33992fa179242478bf1764771525d890c5359f7905378110ab729'
paths = ['pc_port/src/battle_module_stage3_host.inc', 'pc_port/src/battle_shard_setup_host.inc', 'pc_port/src/battle_module_setup_host.inc', 'pc_port/src/battle_texture_host.inc', 'pc_port/src/battle_shard_callbacks_host.inc', 'pc_port/src/battle_module_stage1_host.inc', 'pc_port/src/battle_party_stats_host.inc','pc_port/src/battle_module_stage2_host.inc','pc_port/src/battle_turn_speed_host.inc','pc_port/src/battle_target_list_host.inc', 'pc_port/src/battle_entry_host.inc',
         'pc_port/src/battle_startup_host.inc', 'pc_port/src/battle_display_host.inc',
         'pc_port/src/battle_transition_host.inc', 'pc_port/src/battle_mips_runtime.c',
         'pc_port/src/battle_mips_adapter.c', 'pc_port/src/psyq_compat.c',
         'pc_port/src/retail_leaf_adapters.c', 'pc_port/src/psyq_compat_leaf.inc',
         'pc_port/tests/battle_shard_setup_host_test.c', 'pc_port/tests/run_battle_shard_setup_host_test.sh',
         'pc_port/extern/PsyCross/src/psx/LIBGPU.C', 'pc_port/extern/PsyCross/src/psx/LIBGTE.C',
         'pc_port/extern/PsyCross/src/psx/INLINE_C.C', 'pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp']
(out / 'provenance.json').write_text(json.dumps({
    'scope': 'Retail allocation/initialization vs native bodies with modeled callees; separate integration through actual native SDK/RNG. Not hardware or playable battle acceptance.',
    'allocator_sha256': hashlib.sha256(alloc).hexdigest(),
    'initializer_sha256': hashlib.sha256(init).hexdigest(),
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
for mode in UBSan O0 O2; do
    compiler=gcc
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then
        compiler=clang
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/retail_leaf_adapters.c -o "$OUT/$mode.leaves.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/battle_shard_setup_host_test.c -o "$OUT/$mode.test.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/controller_vblank_service.c -o "$OUT/$mode.vblank.o"
    sdk_objects=()
    for unit in psx/LIBGPU.C psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${unit##*/}
        sdk_objects+=("$OUT/$mode.$name.o")
        g++ "${sdk_common[@]}" "${sdl_flags[@]}" "${flags[@]}" \
            -c "pc_port/extern/PsyCross/src/$unit" -o "$OUT/$mode.$name.o"
    done
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" "$OUT/$mode.leaves.o" \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$OUT/$mode.vblank.o" \
        "${sdk_objects[@]}" -ldl -lcrypto -o "$OUT/$mode.test"
    for suite in sdk model; do
        args=()
        if [ "$suite" = sdk ]; then args=(--sdk); fi
        if ! "$OUT/$mode.test" "${args[@]}" > "$OUT/$mode.$suite.log" 2>&1; then
            cat "$OUT/$mode.$suite.log" >&2
            exit 1
        fi
        cat "$OUT/$mode.$suite.log"
    done
done
python3 - "$OUT" <<'PY'
import pathlib, sys
out = pathlib.Path(sys.argv[1])
runtime = pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body = pathlib.Path('pc_port/src/battle_shard_setup_host.inc').read_text()
mutations = {
    'wrong-allocation': ('0x800b73fcu, 0x10f7c, 1', '0x800b73fcu, 0x10f78, 1'),
    'wrong-self-pointer': ('BH_W32(task + 0x20u, task);', 'BH_W32(task + 0x24u, task);'),
    'discard-initializer-result': ('BH_CALL(0x800b7424u, 0x800b740cu, task);',
                                   'BH_CALL(0x800b7424u, 0x800b740cu, task); entry->cpu->gpr[2] = task;'),
    'wrong-shard-stride': ('BH_FP * 0x7cu;', 'BH_FP * 0x80u;'),
    'wrong-half-stride': ('BH_R32(sp + 0x70u) + 0x87a0u', 'BH_R32(sp + 0x70u) + 0x87a4u'),
    'wrong-random-range': ('0x10624dd3u, 1000)', '0x10624dd3u, 1001)'),
    'unsigned-remainder': ('(uint32_t)((int32_t)random % 1024)', '(random % 1024u)'),
    'wrong-gravity': ('0x70800u - (remainder << 8)', '0x70800u - (remainder << 9)'),
    'wrong-return-pointer': ('entry->cpu->gpr[2] = BH_R32(sp + 0xa0u);', 'entry->cpu->gpr[2] = BH_R32(sp + 0xa0u) + 4u;'),
}
for name, (old, new) in mutations.items():
    assert body.count(old) == 1, (name, body.count(old))
    path = out / (name + '.inc')
    path.write_text(body.replace(old, new))
    anchor = '#include "battle_shard_setup_host.inc"'
    assert runtime.count(anchor) == 1
    (out / (name + '.c')).write_text(runtime.replace(anchor, '#include "' + str(path) + '"'))
anchor = '    void *host = resolved->host;\n'
assert runtime.count(anchor) == 1
(out / 'generic-sdk-call.c').write_text(runtime.replace(anchor, anchor +
    '    if (strcmp(name, "rand") == 0) return 0; /* negative control */\n'))
PY
for mutant in wrong-allocation wrong-self-pointer discard-initializer-result wrong-shard-stride \
              wrong-half-stride wrong-random-range unsigned-remainder wrong-gravity wrong-return-pointer; do
    gcc "${common[@]}" -O2 -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
        -c pc_port/tests/battle_shard_setup_host_test.c -o "$OUT/$mutant.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" "$OUT/O2.leaves.o" \
        "$OUT/O2.cpu.o" "$OUT/O2.vblank.o" "$OUT/O2.LIBGPU.C.o" "$OUT/O2.LIBGTE.C.o" \
        "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -lcrypto -o "$OUT/$mutant.test"
    status=0
    "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" != 1 ] || ! rg -q '^BATTLE SHARD SETUP FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        printf 'BATTLE SHARD SETUP control failed: %s status=%s\n' "$mutant" "$status" >&2
        exit 1
    fi
    printf 'BATTLE SHARD SETUP negative control rejected: %s\n' "$mutant"
done
clang "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all \
    -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/generic-sdk-call.c\"" \
    -c pc_port/tests/battle_shard_setup_host_test.c -o "$OUT/generic-sdk-call.o"
clang++ -no-pie -O1 -fsanitize=undefined -fno-sanitize-recover=all -Wl,--gc-sections \
    "$OUT/generic-sdk-call.o" "$OUT/UBSan.compat.o" "$OUT/UBSan.leaves.o" \
    "$OUT/UBSan.cpu.o" "$OUT/UBSan.vblank.o" "$OUT/UBSan.LIBGPU.C.o" \
    "$OUT/UBSan.LIBGTE.C.o" "$OUT/UBSan.INLINE_C.C.o" "$OUT/UBSan.PsyX_GTE.cpp.o" \
    -ldl -lcrypto -o "$OUT/generic-sdk-call.test"
status=0
UBSAN_OPTIONS=halt_on_error=1:symbolize=0 "$OUT/generic-sdk-call.test" --sdk \
    > "$OUT/generic-sdk-call.log" 2>&1 || status=$?
if [ "$status" != 1 ] || ! rg -q 'runtime error: call to function .*incorrect function type' "$OUT/generic-sdk-call.log"; then
    cat "$OUT/generic-sdk-call.log" >&2
    printf 'BATTLE SHARD SETUP generic SDK control failed: status=%s\n' "$status" >&2
    exit 1
fi
printf 'BATTLE SHARD SETUP negative control rejected: generic-sdk-call\n'
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out = pathlib.Path(sys.argv[1])
for path, digest in json.loads((out / 'provenance.json').read_text())['source_pins'].items():
    assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest() == digest, path
print('BATTLE SHARD SETUP source pins unchanged')
PY
