#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${SPRITE_SCALE_OUT:-$(mktemp -d)}
mkdir -p "$OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
b=Path('disc/SLUS_006.64').read_bytes()
assert sha256(b).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
print('SPRITE SCALE retail bytes SHA256',sha256(b[0x134ac:0x134dc]).hexdigest())
PY
common=(-std=gnu17 -fpermissive -fno-pie -ffunction-sections -fdata-sections -include assert.h -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode"); if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -w -c src/slus_006.64/system/temp1.c -o "$OUT/$mode.source.o"
 gcc "${common[@]}" "${flags[@]}" -c pc_port/tests/sprite_scale_retail_test.c -o "$OUT/$mode.test.o"
 gcc "${common[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode.test"
 "$OUT/$mode.test" disc/SLUS_006.64 | tee "$OUT/$mode.log"
done
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
s=Path('src/slus_006.64/system/temp1.c').read_text();a=s.index('s32 func_80022CAC(');b=s.index('void func_80022CDC(',a);body=s[a:b]
for name,old,new in [('rounding','adj = value + 0x3FF;','adj = value;'),('passthrough','return value;','return 0;'),('signed-product','(s32)((s64)value * factor)','value * factor')]:
 assert old in body
 Path(sys.argv[1],name+'.c').write_text(s[:a]+body.replace(old,new)+s[b:])
PY
for control in rounding passthrough signed-product; do
 mode=O2; flags=(-O2)
 if [ "$control" = signed-product ]; then mode=UBSan; flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${common[@]}" "${flags[@]}" -w -c "$OUT/$control.c" -o "$OUT/$control.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$control.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$control.test"
 rc=0; "$OUT/$control.test" disc/SLUS_006.64 > "$OUT/$control.log" 2>&1 || rc=$?
 test "$rc" != 0
 if [ "$control" = signed-product ]; then rg -q 'signed integer overflow' "$OUT/$control.log"; else rg -q 'SPRITE SCALE FAIL' "$OUT/$control.log"; fi
 echo "SPRITE SCALE CONTROL REJECTED $control"
done
