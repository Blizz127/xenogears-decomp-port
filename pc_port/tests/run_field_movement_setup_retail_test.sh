#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"
CC="${CC:-gcc}"
ubsan_preflight "$CC"
cd "$(dirname "$0")/../.."
OUT="${FIELD_MOVEMENT_OUT:-pc_port/build_native/field_movement_setup}"
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
out=Path(sys.argv[1]);src=Path('src/field/main/misc11.c');s=src.read_text();b=Path('disc/field.bin').read_bytes()
assert sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
h=sha256(b[0x80092808-0x8006faf0:0x80092894-0x8006faf0]).hexdigest()
assert h=='b3bae0ef6b2ce9edb475cc8b8dc8863d055d41f00140d3949bb3c8ff55ef666b'
a=s.index('void func_80092808(');z=s.index('\n}',a)+2
body='#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int16_t s16; typedef int32_t s32;\nextern void *g_FieldScriptVMCurActor,*D_800B06B8;\nint rsin(int); int rcos(int);\n'+s[a:z]+'\n'
(out/'production.c').write_text(body)
(out/'provenance.json').write_text(json.dumps({'field_sha256':sha256(b).hexdigest(),'function_sha256':h,'extracted_sha256':sha256(body.encode()).hexdigest(),'scope':'Verbatim helper, substitute declaration fixtures and trig stubs. No production-TU link, actual trig backend or gameplay. CPU reference only.','source_files':{str(p):sha256(p.read_bytes()).hexdigest() for p in [src,Path('pc_port/tests/field_movement_setup_retail_test.c'),Path('pc_port/tests/run_field_movement_setup_retail_test.sh'),Path('pc_port/src/battle_mips_adapter.c')]}},indent=2)+'\n')
for name,old,new in [('signed-shift','-(cosTerm * 4)','-(cosTerm << 2)'),('scale','-(cosTerm * 4)','-(cosTerm * 3)'),('counter','+= 1;','+= 2;')]:
 (out/(name+'.c')).write_text(body.replace(old,new,1));(out/(name+'.changed')).write_text(str(old in body))
PY
common=(-std=gnu17 -DXENO_PC_PORT -Ipc_port/src)
for mode in O0 O2 UBSan; do
 flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 "$CC" "${common[@]}" "${flags[@]}" "$OUT/production.c" pc_port/tests/field_movement_setup_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
 env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mode" | tee "$OUT/$mode.log"
done
cmp "$OUT/O0.log" "$OUT/O2.log"; cmp "$OUT/O0.log" "$OUT/UBSan.log"
for mutant in signed-shift scale counter;do
 test "$(cat "$OUT/$mutant.changed")" = True
 "$CC" "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all "$OUT/$mutant.c" pc_port/tests/field_movement_setup_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
 if env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1;then echo "MOVEMENT SETUP mutant survived: $mutant" >&2;exit 1;fi
 if [[ "$mutant" == signed-shift ]];then grep -q 'runtime error: left shift of negative' "$OUT/$mutant.log";else grep -q 'MOVEMENT SETUP FAIL' "$OUT/$mutant.log";fi
done
echo 'MOVEMENT SETUP negative controls PASS: signed-shift scale counter'
