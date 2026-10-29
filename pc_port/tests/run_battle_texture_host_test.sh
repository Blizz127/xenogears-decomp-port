#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ulimit -c 0
mkdir -p scratchpad
OUT=${BATTLE_TEXTURE_TEST_OUT:-$(mktemp -d "$PWD/scratchpad/native-texture-test.XXXXXXXX")}
mkdir -p "$OUT"
OUT=$(realpath "$OUT")
export TMPDIR=${TMPDIR:-$OUT/compiler-tmp}
mkdir -p "$TMPDIR"
ubsan_preflight clang
printf 'BATTLE TEXTURE artifacts=%s\n' "$OUT"
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out=pathlib.Path(sys.argv[1])
battle=pathlib.Path('disc/battle.bin').read_bytes()
assert hashlib.sha256(battle).hexdigest() == '1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291'
body=battle[0x8820:0x8a18]
assert hashlib.sha256(body).hexdigest() == '0d9c1fe3bd61c55baf49029bcaeb1df4fcc29a4b194a8aa78ef25aa5ecfe880c'
paths=['pc_port/src/battle_module_stage3_host.inc', 'pc_port/src/battle_texture_host.inc', 'pc_port/src/battle_shard_callbacks_host.inc', 'pc_port/src/battle_module_stage1_host.inc', 'pc_port/src/battle_party_stats_host.inc','pc_port/src/battle_module_stage2_host.inc','pc_port/src/battle_turn_speed_host.inc','pc_port/src/battle_target_list_host.inc','pc_port/src/battle_module_setup_host.inc',
       'pc_port/src/battle_shard_setup_host.inc','pc_port/src/battle_transition_host.inc',
       'pc_port/src/battle_display_host.inc','pc_port/src/battle_startup_host.inc',
       'pc_port/src/battle_entry_host.inc','pc_port/src/battle_mips_runtime.c',
       'pc_port/src/battle_mips_adapter.c','pc_port/src/psyq_compat.c',
       'pc_port/include_shim/psyq/libgte.h',
       'pc_port/src/controller_vblank_service.c','src/slus_006.64/system/temp1f.c',
       'pc_port/extern/PsyCross/src/psx/LIBGPU.C',
       'pc_port/tests/battle_texture_host_test.c','pc_port/tests/run_battle_texture_host_test.sh']
(out/'provenance.json').write_text(json.dumps({
 'scope':'Original caller vs native body with modeled callees; separate real TIM/atlas/SDK wrappers with recording VRAM backend. Not hardware or playable battle acceptance.',
 'body_sha256':hashlib.sha256(body).hexdigest(),
 'source_pins':{p:hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest() for p in paths}
},indent=2)+'\n')
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
    atlas_flags=()
    if [ "$compiler" = gcc ]; then atlas_flags=(-fpermissive); fi
    clang "${common[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
    "$compiler" "${common[@]}" "${flags[@]}" "${atlas_flags[@]}" -w \
        -c src/slus_006.64/system/temp1f.c -o "$OUT/$mode.atlas.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/battle_texture_host_test.c -o "$OUT/$mode.test.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    "$compiler" "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/controller_vblank_service.c -o "$OUT/$mode.vblank.o"
    g++ "${sdk_common[@]}" "${sdl_flags[@]}" "${flags[@]}" \
        -c pc_port/extern/PsyCross/src/psx/LIBGPU.C -o "$OUT/$mode.gpu.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.compat.o" \
        "$OUT/$mode.atlas.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" \
        "$OUT/$mode.vblank.o" "$OUT/$mode.gpu.o" -ldl -lcrypto -o "$OUT/$mode.test"
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
out=pathlib.Path(sys.argv[1])
runtime=pathlib.Path('pc_port/src/battle_mips_runtime.c').read_text()
body=pathlib.Path('pc_port/src/battle_texture_host.inc').read_text()
mutations={
 'wrong-kind-stride': ('* 0x460u);', '* 0x464u);'),
 'wrong-special-slot': ('BT_S2 < 2u', 'BT_S2 < 1u'),
 'wrong-skip': ('BT_V1 != 0x7fu', 'BT_V1 != 0x7eu'),
 'unmasked-entry': ('BT_W8(sp + 0x88u, entry->cpu->gpr[5]);', 'BT_W32(sp + 0x88u, entry->cpu->gpr[5]);'),
 'wrong-seventh-pointer': ('BT_S1 + BT_FP, BT_S1 +', 'BT_S1 + BT_FP + 4u, BT_S1 +'),
 'wrong-clut-word': ('BT_R16(sp + BT_S0 + 0x40u)', 'BT_R16(sp + BT_S0 + 0x44u)'),
 'wrong-pixel-step': ('BT_R32(sp + 0x90u) + 6u', 'BT_R32(sp + 0x90u) + 8u'),
 'lost-frame-pointer': ('BT_FP = BT_R32(sp + 0xc0u);', '(void)BT_R32(sp + 0xc0u);'),
 'short-loop': ('(int32_t)BT_S3 < 3', '(int32_t)BT_S3 < 2'),
 'unguarded-identity': ('BATTLE_TEXTURE_SHA256[i * 2 + 1]) return 0;', 'BATTLE_TEXTURE_SHA256[i * 2 + 1]) break;'),
}
for name,(old,new) in mutations.items():
 assert body.count(old)==1,(name,body.count(old))
 path=out/(name+'.inc');path.write_text(body.replace(old,new))
 anchor='#include "battle_texture_host.inc"'
 assert runtime.count(anchor)==1
 (out/(name+'.c')).write_text(runtime.replace(anchor,'#include "'+str(path)+'"'))
anchor='    if (strcmp(resolved->name, "OpenTIM") == 0) {\n'
assert runtime.count(anchor)==1
(out/'generic-open-tim.c').write_text(runtime.replace(anchor, '    if (strcmp(resolved->name, "OpenTIM") == 0) return 0;\n'+anchor))
anchor='int (*load_image)(RECT16 *, u_long *) = (int (*)(RECT16 *, u_long *))host_function;'
assert runtime.count(anchor)==1
(out/'untyped-load-image.c').write_text(runtime.replace(anchor,'int (*load_image)(void *, void *) = (int (*)(void *, void *))host_function;'))
PY
for mutant in wrong-kind-stride wrong-special-slot wrong-skip unmasked-entry wrong-seventh-pointer \
              wrong-clut-word wrong-pixel-step lost-frame-pointer short-loop unguarded-identity; do
    gcc "${common[@]}" -O2 -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
        -c pc_port/tests/battle_texture_host_test.c -o "$OUT/$mutant.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.compat.o" \
        "$OUT/O2.atlas.o" "$OUT/O2.cpu.o" "$OUT/O2.vblank.o" "$OUT/O2.gpu.o" \
        -ldl -lcrypto -o "$OUT/$mutant.test"
    status=0
    "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" != 1 ] || ! rg -q '^BATTLE TEXTURE FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        printf 'BATTLE TEXTURE control failed: %s status=%s\n' "$mutant" "$status" >&2
        exit 1
    fi
    printf 'BATTLE TEXTURE negative control rejected: %s\n' "$mutant"
done
for mutant in generic-open-tim untyped-load-image; do
    clang "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all \
        -Wall -Wextra -Werror "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" \
        -c pc_port/tests/battle_texture_host_test.c -o "$OUT/$mutant.o"
    clang++ -no-pie -O1 -fsanitize=undefined -fno-sanitize-recover=all -Wl,--gc-sections \
        "$OUT/$mutant.o" "$OUT/UBSan.compat.o" "$OUT/UBSan.atlas.o" \
        "$OUT/UBSan.cpu.o" "$OUT/UBSan.vblank.o" "$OUT/UBSan.gpu.o" \
        -ldl -lcrypto -o "$OUT/$mutant.test"
    status=0
    UBSAN_OPTIONS=halt_on_error=1:symbolize=0 "$OUT/$mutant.test" --sdk > "$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" != 1 ] || ! rg -q 'runtime error: call to function .*incorrect function type' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        printf 'BATTLE TEXTURE SDK control failed: %s status=%s\n' "$mutant" "$status" >&2
        exit 1
    fi
    printf 'BATTLE TEXTURE negative control rejected: %s\n' "$mutant"
done
python3 - "$OUT" <<'PY'
import hashlib, json, pathlib, sys
out=pathlib.Path(sys.argv[1])
for path,digest in json.loads((out/'provenance.json').read_text())['source_pins'].items():
 assert hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()==digest,path
print('BATTLE TEXTURE source pins unchanged')
PY
