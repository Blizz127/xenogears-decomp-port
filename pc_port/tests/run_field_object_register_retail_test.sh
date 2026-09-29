#!/usr/bin/env bash
# Full production misc6 TU vs the pinned retail func_800A1364 bytes, with
# explicit script-argument / sprite-bind / pose-reset spies on both sides.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
SOURCE=${FIELD_OBJECT_REGISTER_SOURCE:-src/field/main/misc6.c}
OUT=${FIELD_OBJECT_REGISTER_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/xeno-field-object-register.XXXXXXXX")}
mkdir -p "$OUT"
echo "FIELD OBJECT REGISTER OUTPUT $OUT"
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
source=Path(sys.argv[1]);out=Path(sys.argv[2]);disc=Path('disc/field.bin');raw=disc.read_bytes()
assert sha256(raw).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
assert sha256(raw[0x31874:0x31A00]).hexdigest()=='abf8efc25897d54dc9dd9b7fa2d38ed2e464836d332eb3cbb9b506e598b7be98'
paths=[source,disc,Path('pc_port/tests/field_object_register_retail_test.c'),Path('pc_port/tests/run_field_object_register_retail_test.sh'),Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/src/battle_mips_adapter.h'),Path('include/field/actor.h'),Path('include/common.h'),Path('pc_port/build_port.sh')]
(out/'pins.json').write_text(json.dumps({'source_pins':{str(p.resolve()):sha256(p.read_bytes()).hexdigest() for p in paths},'retail_function':{'entry':'800A1364','end':'800A14F0','raw_offset':'31874','bytes':396,'instructions':99},'scope':'full production TU body; FieldScriptVMGetArgument/func_80076AC0/func_800A0C94 are boundary spies, not the script VM, sprite binder or pose reset; object slots 0..4 only because retail keeps only five flag bytes ahead of the counter','clang_compatibility':'test-only forward declarations of six later full-TU definitions avoid conflicting implicit declarations; the production TU is compiled with -fno-inline-functions and func_800A0C94 weakened so the same-TU pose-reset call reaches the spy; no function body or tested helper prototype replaced'},indent=2)+'\n')
(out/'native-prototypes.h').write_text('''/* Test-only forward declarations of existing later full-TU definitions. */
#include "common.h"
void func_8009E574(s16, s16);
void func_8009E810(s16);
void func_8009F5F4(void);
void func_8009FD10(int);
void func_800A0C94(void);
void func_800A0D3C(void);
''')
PY
COMMON=(-include assert.h -include "$OUT/native-prototypes.h" -std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
build_source() {
 # $1 = C file, $2 = object, remaining = flags. func_800A0C94 lives in the same
 # TU as the function under test; keep the call a call (clang would otherwise
 # inline the same-TU body at -O1/-O2) and weaken it so the spy is the definition.
 local src=$1 obj=$2; shift 2
 clang "${COMMON[@]}" "$@" -fno-inline-functions -Wno-everything -c "$src" -o "$obj"
 objcopy --weaken-symbol=func_800A0C94 "$obj"
}
for mode in O0 O2 UBSan; do
 flags=(-"$mode");if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 build_source "$SOURCE" "$OUT/$mode.source.o" "${flags[@]}"
 clang "${COMMON[@]}" "${flags[@]}" -c pc_port/tests/field_object_register_retail_test.c -o "$OUT/$mode.test.o"
 clang "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
 "$OUT/$mode" disc/field.bin > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log";exit 1; }
 cat "$OUT/$mode.log"
done
python3 - "$SOURCE" "$OUT" <<'PY'
import sys,json
from pathlib import Path
s=Path(sys.argv[1]).read_text();out=Path(sys.argv[2]);a=s.index('void func_800A1364(void) {');b=s.index('void func_800A14F0',a);body=s[a:b]
controls={}
for name,old,new in [('status-set','& 0xF07F) | 0x200;','& 0xF07F) | 0x100;'),('ip-advance','scriptInstructionPointer += 3;','scriptInstructionPointer += 2;'),('actor-clear','.status &= 0xFFDF;','.status &= 0xFFEF;'),('flags4','| 0x2000) & ~0x800;','| 0x2000) & ~0x400;'),('id-scale','spriteId << 1;','spriteId << 2;'),('slot-shift','& 7) << 13);','& 7) << 12);'),('count-increment','D_800B2264++;','D_800B2264 += 2;')]:
 assert body.count(old)==1,(name,body.count(old))
 path=out/(name+'.c');path.write_text(s[:a]+body.replace(old,new)+s[b:]);controls[name]=str(path)
(out/'controls.json').write_text(json.dumps(controls,indent=2)+'\n')
PY
for control in status-set ip-advance actor-clear flags4 id-scale slot-shift count-increment; do
 build_source "$OUT/$control.c" "$OUT/$control.source.o" -O2
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$control.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/$control"
 rc=0
 "$OUT/$control" disc/field.bin > "$OUT/$control.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! rg -q '^FIELD OBJECT REGISTER FAIL ' "$OUT/$control.log";then cat "$OUT/$control.log";echo "FIELD OBJECT REGISTER control failed to detect defect: $control rc=$rc" >&2;exit 1;fi
 echo "FIELD OBJECT REGISTER CONTROL REJECTED $control"
done
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
pins=json.loads((Path(sys.argv[1])/'pins.json').read_text())
for path,wanted in pins['source_pins'].items():assert sha256(Path(path).read_bytes()).hexdigest()==wanted,path
PY
