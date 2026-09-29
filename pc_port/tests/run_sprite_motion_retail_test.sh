#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${SPRITE_MOTION_OUT:-$(mktemp -d)}
mkdir -p "$OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
b=Path('disc/SLUS_006.64').read_bytes()
assert sha256(b).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
print('SPRITE MOTION retail bytes SHA256',sha256(b[0x1332c:0x13544]).hexdigest())
PY
common=(-std=gnu17 -fpermissive -fno-pie -ffunction-sections -fdata-sections -include assert.h -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
weaken_matching() {
 local object=$1 symbol reason condition
 while IFS='|' read -r symbol reason condition; do
  [[ -z "$symbol" || "$symbol" == \#* ]] && continue
  objcopy --weaken-symbol="$symbol" "$object"
 done < pc_port/port_owned_overrides.txt
 for symbol in func_80022CAC func_80022B2C func_80022CDC; do
  nm -g --defined-only "$object" | awk -v s="$symbol" '$3==s && $2=="T" {found=1} END {exit !found}'
 done
}
for mode in ${SPRITE_MOTION_MODES:-O0 O2 UBSan}; do
 flags=(-"$mode"); if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -w -c pc_port/src/game_overrides.c -o "$OUT/$mode.port.o"
 gcc "${common[@]}" "${flags[@]}" -w -c src/slus_006.64/system/temp1.c -o "$OUT/$mode.source.o"
 weaken_matching "$OUT/$mode.source.o"
 gcc "${common[@]}" "${flags[@]}" -c pc_port/tests/sprite_motion_retail_test.c -o "$OUT/$mode.test.o"
 gcc "${common[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.port.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode.test"
 "$OUT/$mode.test" disc/SLUS_006.64 | tee "$OUT/$mode.log"
done
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
s=Path('src/slus_006.64/system/temp1.c').read_text();a=s.index('void func_80022B2C(');b=s.index('extern void func_8001D2B0',a);body=s[a:b]
controls={
 'floor-query':('func_800BA8F4(p);','/* missing query */'),
 'bounce-coefficient':('& 0x3FF','& 0x1FF'),
 'gravity':('*(u32*)(p + 0x10) += *(u32*)(p + 0x1C);','*(u32*)(p + 0x10) += 0;'),
 'horizontal-axis':('pSprite + 0x14) >> 4','pSprite + 0x0C) >> 4'),
 'signed-shift':('(u32)val << 4','val << 4')}
for name,(old,new) in controls.items():
 assert old in body,name
 Path(sys.argv[1],name+'.c').write_text(s[:a]+body.replace(old,new)+s[b:])
PY
for control in floor-query bounce-coefficient gravity horizontal-axis signed-shift; do
 mode=O2;flags=(-O2)
 if [ "$control" = signed-shift ]; then mode=UBSan;flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -w -c "$OUT/$control.c" -o "$OUT/$control.o"
 weaken_matching "$OUT/$control.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$control.o" "$OUT/$mode.port.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$control.test"
 rc=0;"$OUT/$control.test" disc/SLUS_006.64 > "$OUT/$control.log" 2>&1 || rc=$?
 test "$rc" != 0
 if [ "$control" = signed-shift ]; then rg -q 'left shift of negative value' "$OUT/$control.log";else rg -q 'SPRITE MOTION FAIL' "$OUT/$control.log";fi
 echo "SPRITE MOTION CONTROL REJECTED $control"
done
