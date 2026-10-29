#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"
CC="${CC:-clang}"
ubsan_preflight "$CC"
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT="${FIELD_TURN_OUT:-pc_port/build_native/field_turn}"
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
out=Path(sys.argv[1]);src=Path('src/field/main/misc8.c');s=src.read_text()
b=Path('disc/field.bin').read_bytes()
assert sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
code=b[0x800831f4-0x8006faf0:0x80083288-0x8006faf0]
assert sha256(code).hexdigest()=='9895918bb0665217e57a3ad6c55200fc01837b14cab405210cdd7e74016d6a58'
a=s.index('void func_800831F4(');end=s.index('\n}',a)+2
body='#include "field_turn_retail_test.h"\n'+s[a:end]+'\n'
(out/'production.c').write_text(body)
pins={'field_sha256':sha256(b).hexdigest(),'retail_function_sha256':sha256(code).hexdigest(),'extracted_source_sha256':sha256(body.encode()).hexdigest(),
 'scope':'Verbatim extracted helper, callee stub; no production TU link, proven runtime reachability or playable acceptance.',
 'source_files':{str(p):sha256(p.read_bytes()).hexdigest() for p in [src,Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/src/battle_mips_adapter.h'),*Path('pc_port/tests').glob('*field_turn*')]}}
(out/'provenance.json').write_text(json.dumps(pins,indent=2)+'\n')
mutations={
 'wrong-actor':('0x104), pFieldActor);','0x104), pFieldActor == 0 ? pFieldActor : pActor);'),
 'direction':('flags & 1','flags & 2'),
 'mask':('newAngle &= 0xFFF;','newAngle &= 0xFFFF;'),
 'inactive':('val30 != 0 || val38 != 0','val30 != 0 && val38 != 0'),
 'locked':('if (!(curAngle & 0x8000))','if (curAngle != 0)'),
 'stale-angle':('*(u16*)(pActor + 0x106) | 0x8000','newAngle | 0x8000'),
 'pointer-width':('0x104), pFieldActor);','0x104), (void*)(uintptr_t)(u32)(uintptr_t)pFieldActor);'),
}
# Original signature is rejected by the canonical contract before mutations run.
for name,(old,new) in mutations.items():
 (out/(name+'.c')).write_text(body.replace(old,new,1))
 (out/(name+'.changed')).write_text(str(old in body))
PY
common=(-std=gnu17 -Wall -Wextra -Werror -DXENO_PC_PORT -Ipc_port/tests -Ipc_port/src)
for mode in O0 O2 UBSan; do
 flags=(-"$mode")
 if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 "$CC" "${common[@]}" "${flags[@]}" "$OUT/production.c" pc_port/tests/field_turn_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
 env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mode" disc/field.bin | tee "$OUT/$mode.log"
done
cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSan.log"
for mutant in wrong-actor direction mask inactive locked stale-angle pointer-width; do
 test "$(cat "$OUT/$mutant.changed")" = True
 "$CC" "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all "$OUT/$mutant.c" pc_port/tests/field_turn_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
 if env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mutant" disc/field.bin >"$OUT/$mutant.log" 2>&1; then
   echo "FIELD TURN mutant survived: $mutant" >&2; exit 1
 fi
 grep -q 'FIELD TURN FAIL' "$OUT/$mutant.log"
done
echo 'FIELD TURN negative controls PASS: wrong-actor direction mask inactive locked stale-angle pointer-width'
echo "FIELD TURN receipts: $OUT"
