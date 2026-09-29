#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/battle_sprite_hooks_retail_test.XXXXXXXX)
echo "BATTLE SPRITE HOOKS artifacts: $OUT"
bridge_elf=(); if [ -f build/out/slus_006.64.elf ]; then bridge_elf+=(--elf build/out/slus_006.64.elf); fi
# Use the same native symbol classification as the shipped port.
# Retail addresses still come exclusively from the symbol maps.
bridge_elf+=(--host-elf pc_port/build_native/xeno-port)
python3 tools/scripts/gen_battle_bridge_map.py "${bridge_elf[@]}" --symbols config/symbol_addrs.slus_006.64.txt \
 --symbols linker/undefined_funcs_auto.battle.txt --symbols linker/undefined_syms_auto.battle.txt \
 --symbols config/symbol_addrs.battle.txt \
 --out "$OUT/battle_bridge_map.inc"
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
out=Path(sys.argv[1]); battle=Path('disc/battle.bin').read_bytes(); base=0x8006faf0
pins={}
for name,start,end,expected in [
 ('gravity',0x800ba8f4,0x800ba984,'9b851798e13ec17533ca4f873df75a301075eb9c33a3f653552ad059146df5ed'),
 ('child',0x800bc158,0x800bc2f0,'1f42d3930dcfaf1335bb20084a37516c7e61589d4621cb9cd05108dc251f315f')]:
 data=battle[start-base:end-base]; actual=sha256(data).hexdigest(); assert actual==expected,(name,actual)
 pins[name]={'start':hex(start),'end':hex(end),'sha256':actual}
source_paths=['pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c',
 'tools/scripts/gen_battle_bridge_map.py','config/symbol_addrs.slus_006.64.txt',
 'linker/undefined_funcs_auto.battle.txt','linker/undefined_syms_auto.battle.txt',
 'config/symbol_addrs.battle.txt','pc_port/tests/battle_sprite_hooks_retail_test.c',
 'pc_port/tests/run_battle_sprite_hooks_retail_test.sh','pc_port/src/game_overrides.c']
pins['source_files']={p:sha256(Path(p).read_bytes()).hexdigest() for p in source_paths}
pins['generated_bridge_map_sha256']=sha256((out/'battle_bridge_map.inc').read_bytes()).hexdigest()
s=Path('pc_port/src/game_overrides.c').read_text()
def fn(sig):
 start=s.index(sig);end=s.index('\n}',start)+2;return s[start:end]
body='#include <stdio.h>\n#include <stdlib.h>\n#include <stdint.h>\ntypedef uint8_t u8;\ntypedef int32_t s32;\nextern int PcPort_BattleMipsDispatchCallback(uint32_t,void*);\n'+fn('void func_800BA8F4(')+'\n'+fn('void func_800BC158(')+'\n'
(out/'production.c').write_text(body);pins['verbatim_extracted_functions_sha256']=sha256(body.encode()).hexdigest()
(out/'noop.c').write_text(body[:body.index('void func_800BA8F4(')]+'''void func_800BA8F4(void *p){(void)p;}
void func_800BC158(void *p){(void)p;}
''')
(out/'wrong-target.c').write_text(body.replace('0x800ba8f4u','0x800ba8f8u').replace('0x800bc158u','0x800bc15cu'))
(out/'wrong-argument.c').write_text(body.replace('0x800ba8f4u, pSpriteData','0x800ba8f4u, (u8*)pSpriteData + 4').replace('0x800bc158u, pWrapper','0x800bc158u, (u8*)pWrapper + 4'))
(out/'provenance.json').write_text(json.dumps(pins,indent=2)+'\n')
PY
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
 -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$OUT" -Ipc_port/extern/PsyCross/include
 -Ipc_port/extern/PsyCross/include/psx -Wall -Wextra -Werror)
build_and_run() {
 local mode=$1 opt=$2 sanitize=$3
 clang "${common[@]}" "$opt" $sanitize -c pc_port/tests/battle_sprite_hooks_retail_test.c -o "$OUT/test-$mode.o"
 clang -std=gnu17 -fno-pie -ffunction-sections -fdata-sections "$opt" $sanitize -Wall -Wextra -Werror \
  -c pc_port/src/battle_mips_adapter.c -o "$OUT/cpu-$mode.o"
 clang -std=gnu17 -fno-pie -ffunction-sections -fdata-sections "$opt" $sanitize -c "$OUT/production.c" -o "$OUT/prod-$mode.o"
 clang -no-pie $sanitize -Wl,--gc-sections -Wl,--export-dynamic "$OUT/test-$mode.o" "$OUT/cpu-$mode.o" "$OUT/prod-$mode.o" -ldl -o "$OUT/test-$mode"
 "$OUT/test-$mode" | tee "$OUT/$mode.log"
}
build_and_run O0 -O0 ''
build_and_run O2 -O2 ''
build_and_run UBSan -O1 '-fsanitize=undefined -fno-sanitize=function -fno-sanitize-recover=all'
for inactive in inactive-gravity inactive-child; do
 if "$OUT/test-O0" "$inactive" >"$OUT/$inactive.log" 2>&1; then
  echo "inactive-runtime negative control unexpectedly returned" >&2; exit 1
 fi
done
for mutant in noop wrong-target wrong-argument; do
 clang -std=gnu17 -fno-pie -ffunction-sections -fdata-sections -O0 -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -no-pie -Wl,--gc-sections -Wl,--export-dynamic "$OUT/test-O0.o" "$OUT/cpu-O0.o" "$OUT/$mutant.o" -ldl -o "$OUT/test-$mutant"
 if "$OUT/test-$mutant" >"$OUT/$mutant.log" 2>&1; then
  echo "$mutant negative control unexpectedly passed" >&2; exit 1
 fi
 echo "BATTLE SPRITE HOOKS mutant rejected: $mutant"
done
