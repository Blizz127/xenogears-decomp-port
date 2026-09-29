#!/usr/bin/env bash
# Full production misc2 TU vs pinned retail function, with explicit SDK spies.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
SOURCE=${FIELD_AXIS_MATRIX_SOURCE:-src/field/main/misc2.c}
OUT=${FIELD_AXIS_MATRIX_OUT:-$(mktemp -d /tmp/xeno-field-axis-matrix.XXXXXXXX)}
mkdir -p "$OUT"
echo "FIELD AXIS MATRIX OUTPUT $OUT"
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
source=Path(sys.argv[1]);out=Path(sys.argv[2]);disc=Path('disc/field.bin');raw=disc.read_bytes()
assert sha256(raw).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
assert sha256(raw[0x5ef4:0x6018]).hexdigest()=='b4f76d0b3dd362838d4eb82c2dfefab068628db24f11de43f7fe0504c8b04cf3'
assert raw[0x80:0x90]==bytes.fromhex('00000000000000000010000000000000')
paths=[source,disc,Path('pc_port/tests/field_axis_matrix_retail_test.c'),Path('pc_port/tests/run_field_axis_matrix_retail_test.sh'),Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/src/battle_mips_adapter.h'),Path('pc_port/include_shim/psyq/libgte.h'),Path('pc_port/extern/PsyCross/include/psx/libgte.h'),Path('include/common.h'),Path('pc_port/build_port.sh')]
(out/'pins.json').write_text(json.dumps({'source_pins':{str(p.resolve()):sha256(p.read_bytes()).hexdigest() for p in paths},'retail_function':{'entry':'800759E4','end':'80075B08','raw_offset':'5EF4','bytes':292},'scope':'full production TU and seed; explicit OuterProduct12/VectorNormal boundary spies, not SDK math or rendering oracle','clang_compatibility':'test-only forward declarations of four unrelated later full-TU definitions avoid conflicting implicit declarations; no function body or tested helper prototype replaced'},indent=2)+'\n')
(out/'native-prototypes.h').write_text('''/* Test-only forward declarations of existing later full-TU definitions. */
#include "common.h"
#include "psyq/libgte.h"
void FieldMatrixCopy(MATRIX *,MATRIX *);
void FieldMatrixCopyTranslation(MATRIX *,MATRIX *);
void FieldMatrixCopyTransform(MATRIX *,MATRIX *);
void func_80281B00(void *);
''')
PY
COMMON=(-include assert.h -include "$OUT/native-prototypes.h" -std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode");if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 clang "${COMMON[@]}" "${flags[@]}" -Wno-everything -c "$SOURCE" -o "$OUT/$mode.source.o"
 clang "${COMMON[@]}" "${flags[@]}" -c pc_port/tests/field_axis_matrix_retail_test.c -o "$OUT/$mode.test.o"
 clang "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
 "$OUT/$mode" disc/field.bin > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log";exit 1; }
 cat "$OUT/$mode.log"
done
python3 - "$SOURCE" "$OUT" <<'PY'
import sys,json
from pathlib import Path
s=Path(sys.argv[1]).read_text();out=Path(sys.argv[2]);a=s.index('/* Retail 800759E4');b=s.index('void func_80075B08',a);body=s[a:b]
controls={}
for name,old,new in [('wrong-seed','{ 0, 0, 0x1000, 0 }','{ 0, 0, 0x1001, 0 }'),('wrong-axis-row','matrix->m[1][1] = axis->vy;','matrix->m[1][1] = axis->vx;'),('swapped-cross','OuterProduct12(&right,axis,&temporary);','OuterProduct12(axis,&right,&temporary);'),('missing-normal','VectorNormal(&temporary,&up);','up = temporary;')]:
 assert body.count(old)==1,(name,body.count(old))
 path=out/(name+'.c');path.write_text(s[:a]+body.replace(old,new)+s[b:]);controls[name]=str(path)
(out/'controls.json').write_text(json.dumps(controls,indent=2)+'\n')
PY
for control in wrong-seed wrong-axis-row swapped-cross missing-normal; do
 clang "${COMMON[@]}" -O2 -Wno-everything -c "$OUT/$control.c" -o "$OUT/$control.source.o"
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$control.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/$control"
 rc=0
 "$OUT/$control" disc/field.bin > "$OUT/$control.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! rg -q '^FIELD AXIS FAIL ' "$OUT/$control.log";then cat "$OUT/$control.log";echo "FIELD AXIS MATRIX control failed to detect defect: $control rc=$rc" >&2;exit 1;fi
 echo "FIELD AXIS CONTROL REJECTED $control"
done
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
pins=json.loads((Path(sys.argv[1])/'pins.json').read_text())
for path,wanted in pins['source_pins'].items():assert sha256(Path(path).read_bytes()).hexdigest()==wanted,path
PY
