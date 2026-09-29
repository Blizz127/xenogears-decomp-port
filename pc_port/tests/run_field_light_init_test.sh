#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_light_init
mkdir -p "$OUT"
python3 - <<'PY'
from pathlib import Path
import hashlib,re
b=Path('disc/field.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
for name,lo,hi in [('misc3/FieldLoad',0x80071618,0x8007167c),('main/func_80077844',0x80077844,0x80077884)]:
 a=Path('asm/field/matchings/main/'+name+'.s').read_text()
 raw=b''.join(bytes.fromhex(w) for addr,w in re.findall(r'/\* [0-9A-F]+ ([0-9A-F]+) ([0-9A-F]{8}) \*/',a) if lo<=int(addr,16)<hi)
 assert raw==b[lo-0x8006faf0:hi-0x8006faf0]
s=Path('src/field/main/misc3.c').read_text()
calls=re.findall(r'^    func_80077844\([^\n]+;',s,re.M)
assert len(calls)==2
prototype=re.search(r'extern void func_80077844\([^;]+;',s).group(0)
Path('pc_port/build_native/field_light_init/light_init_calls.inc').write_text(prototype+'\nstatic void native_init(unsigned char *D_800B223C) {\n'+'\n'.join(calls)+'\n}\n')
PY
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT
 -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h
 -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
 -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode")
 if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc "${BASE[@]}" "${flags[@]}" -fpermissive -w -c src/field/main/main.c -o "$OUT/$mode.owner.o"
 gcc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src -I"$OUT" -c pc_port/tests/field_light_init_test.c -o "$OUT/$mode.test.o"
 gcc -std=gnu17 "${flags[@]}" -Ipc_port/src -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.owner.o" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
 "$OUT/$mode" | tee "$OUT/$mode.log"
done
python3 - <<'PY'
from pathlib import Path
p=Path('pc_port/build_native/field_light_init')
s=(p/'light_init_calls.inc').read_text()
mutations={
 'missing_red_light':('0x800, 0, 0, 0x800, 0, 0, 0x800, 0, 0','0x800, 0, 0, 0x800, 0, 0, 0, 0, 0'),
 'ninth_value':('0x800, 0, 0, 0x800, 0, 0, 0x800, 0, 0','0x800, 0, 0, 0x800, 0, 0, 0x800, 0, 1'),
 'direction_sign':('-0xFC1','0xFC1'),
 'destination':('D_800B223C - 0x20','D_800B223C - 0x1E'),
}
for name,(old,new) in mutations.items():
 assert s.count(old)==1
 (p/name).mkdir(exist_ok=True)
 (p/name/'light_init_calls.inc').write_text(s.replace(old,new))
PY
for mutant in missing_red_light ninth_value direction_sign destination; do
 gcc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -I"$OUT/$mutant" -c pc_port/tests/field_light_init_test.c -o "$OUT/$mutant/test.o"
 clang -no-pie -Wl,--gc-sections "$OUT/O2.owner.o" "$OUT/$mutant/test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant/test"
 if "$OUT/$mutant/test" >"$OUT/$mutant.log" 2>&1; then
  echo "FAIL negative control survived: $mutant" >&2; exit 1
 fi
 grep -q '^FAIL matrix ' "$OUT/$mutant.log"
done
echo 'FIELD LIGHT INITIALIZATION negative controls PASS count=4'
