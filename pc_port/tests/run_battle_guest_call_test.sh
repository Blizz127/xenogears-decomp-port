#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
SOURCE=${BATTLE_GUEST_CALL_SOURCE:-"$PWD/pc_port/src/battle_mips_runtime.c"}
SOURCE=$(realpath "$SOURCE")
OUT=${BATTLE_GUEST_CALL_OUT:-$(mktemp -d /tmp/xeno-guest-call-test.XXXXXXXX)}
mkdir -p "$OUT"
printf 'BATTLE GUEST CALL source=%s artifacts=%s\n' "$SOURCE" "$OUT"
python3 - "$OUT" "$SOURCE" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
out=Path(sys.argv[1]);source=Path(sys.argv[2])
payload=Path('disc/battle_command_file1.bin').read_bytes()
assert sha256(payload).hexdigest()=='64668d85bf48dea46cf6d5f04b38e38ca887877cfef53961f0c26cd2d363b670'
assert payload[0x1ce8:0x1cec]==bytes.fromhex('b0ffbd27')
assert sha256(payload[0x1ce8:0x21d4]).hexdigest()=='1f11c9ac7117c0d702c6829cb3fb57806d73c6413eaec5f9fbf1ecca2b427a2e'
asm=Path('asm/battle_command_file1/matchings/message_controller/xeno_battle_command_file1_controller.s')
if not asm.exists():  # the split now emits the controller as one asm segment
    asm=Path('asm/battle_command_file1/message_controller.s')
paths=[source,Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/tests/battle_guest_call_test.c'),Path('pc_port/tests/run_battle_guest_call_test.sh'),asm]
h=Path('pc_port/src/battle_mips_runtime_internal.h')
if h.exists():paths.append(h)
(out/'provenance.json').write_text(json.dumps({'scope':'production native-to-guest call service; synthetic MIPS callees; no controller substitution','retail_controller_frame_bytes':80,'source_pins':{str(p.resolve()):sha256(p.read_bytes()).hexdigest() for p in paths}},indent=2)+'\n')
PY
bridge_elf=()
if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" \
    --symbols config/symbol_addrs.slus_006.64.txt \
    --symbols linker/undefined_funcs_auto.battle.txt \
    --symbols linker/undefined_syms_auto.battle.txt \
    --symbols config/symbol_addrs.battle.txt --out "$OUT/battle_bridge_map.inc"
COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT"
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in ${BATTLE_GUEST_CALL_MODES:-O0 O2 UBSan}; do
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
        echo "BATTLE GUEST CALL UBSan regime: clang (-fsanitize=undefined; gcc cannot link libubsan on this host)"
    fi
    clang "${COMMON[@]}" "${flags[@]}" -w -c pc_port/src/psyq_compat.c -o "$OUT/$mode.compat.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$SOURCE\"" -c pc_port/tests/battle_guest_call_test.c -o "$OUT/$mode.test.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/controller_vblank_service.c -o "$OUT/$mode.vblank.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.vblank.o" "$OUT/$mode.compat.o" \
        "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -ldl -o "$OUT/$mode"
    status=0
    "$OUT/$mode" >"$OUT/$mode.log" 2>&1 || status=$?
    cat "$OUT/$mode.log"
    if [ "$status" -ne 0 ] || ! rg -q '^BATTLE GUEST CALL PASS ' "$OUT/$mode.log"; then exit 1; fi
done
python3 - "$OUT" "$SOURCE" <<'PYCONTROL'
from pathlib import Path
from hashlib import sha256
import json,sys
out=Path(sys.argv[1]);s=Path(sys.argv[2]).read_text()
a=s.index('int PcPort_BattleMipsCallGuest(');b=s.index('static int run_guest_callback(',a);body=s[a:b]
changes={
 'wrong-frame':('sp -= 0x50u;','sp -= 0x48u;'),
 'wrong-stack-slot':('sp + 0x10u + (i - 4u) * 4u','sp + 0x14u + (i - 4u) * 4u'),
 'drop-gp':('cpu.gpr[0] = 0;','cpu.gpr[0] = 0; cpu.gpr[28] = 0;'),
 'missing-context-restore':('runtime->bridge_cpu = caller;','runtime->bridge_cpu = &cpu;'),
 'aliased-args-not-snapshotted':('                          words[i])','                          args[i])'),
 'masking-ram-target':('target >= 0x80200000u','target >= 0x80300000u'),
}
manifest={}
for name,(old,new) in changes.items():
 assert body.count(old)==1,(name,body.count(old))
 code=s[:a]+body.replace(old,new,1)+s[b:]
 (out/(name+'.c')).write_text(code);manifest[name]=sha256(code.encode()).hexdigest()
(out/'negative-controls.json').write_text(json.dumps(manifest,indent=2)+'\n')
PYCONTROL
for mutant in wrong-frame wrong-stack-slot drop-gp missing-context-restore aliased-args-not-snapshotted masking-ram-target; do
    clang "${COMMON[@]}" -O2 -Wall -Wextra -Werror \
        "-DBATTLE_RUNTIME_SOURCE=\"$OUT/$mutant.c\"" -c pc_port/tests/battle_guest_call_test.c -o "$OUT/$mutant.test.o"
    clang -no-pie -O2 -Wl,--gc-sections "$OUT/O2.vblank.o" "$OUT/O2.compat.o" "$OUT/$mutant.test.o" "$OUT/O2.cpu.o" -ldl -o "$OUT/$mutant"
    status=0
    "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1 || status=$?
    if [ "$status" -ne 1 ] || ! rg -q '^BATTLE GUEST CALL FAIL ' "$OUT/$mutant.log"; then
        cat "$OUT/$mutant.log" >&2
        printf 'BATTLE GUEST CALL CONTROL FAILURE %s status=%s\n' "$mutant" "$status" >&2
        exit 1
    fi
    printf 'BATTLE GUEST CALL control rejected %s\n' "$mutant"
done
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
p=json.loads((Path(sys.argv[1])/'provenance.json').read_text())
for path,expected in p['source_pins'].items():assert sha256(Path(path).read_bytes()).hexdigest()==expected,path
print('BATTLE GUEST CALL source pins unchanged')
PY
