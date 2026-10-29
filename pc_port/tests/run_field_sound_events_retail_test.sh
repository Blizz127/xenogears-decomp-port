#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"
CC="${CC:-clang}"
ubsan_preflight "$CC"
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT="${FIELD_SOUND_EVENTS_OUT:-pc_port/build_native/field_sound_events}"
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
out=Path(sys.argv[1]); src=Path('src/field/main/misc8.c');s=src.read_text()
b=Path('disc/field.bin').read_bytes()
assert sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
code=b[0x80085678-0x8006faf0:0x80085738-0x8006faf0]
assert sha256(code).hexdigest()=='19176a32111b2bcf367d4beb621ad2996fea260f23885a98e963b37ef88a71e8'
a=s.index('void func_80085678(void) {');end=s.index('\n}',a)+2
# Include the actual production declarations as well as the verbatim function.
start=s.rindex('extern s16 D_800C3A38;',0,a)
body='#include "field_sound_events_retail_test.h"\n'+s[start:end]+'\n'
(out/'production.c').write_text(body)
pins={'field_sha256':sha256(b).hexdigest(),'retail_function_sha256':sha256(code).hexdigest(),
 'extracted_source_sha256':sha256(body.encode()).hexdigest(),
 'scope':'Verbatim extracted production helper; not production-TU linkage, audio rendering or playable acceptance.',
 'source_files':{str(p):sha256(p.read_bytes()).hexdigest() for p in [src,Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/src/battle_mips_adapter.h'),*Path('pc_port/tests').glob('*field_sound_events*')]}}
(out/'provenance.json').write_text(json.dumps(pins,indent=2)+'\n')
mutations={
 'channel':('func_80039EC4(param, hi);','func_80039EC4(param, hi & 0);'),
 'bank-offset':('(u8*)D_800B235C + 0x14','(u8*)D_800B235C + 0x12'),
 'deadline':(' > D_800B06A0',' >= D_800B06A0'),
 'narrow-state':('if (D_800C3A38 == 0xFF)','if ((u8)D_800C3A38 == 0xFF)'),
 'counter':('D_800C3A64 + 1','D_800C3A64 + 2'),
 'header-pointer':('(u32)*(u16*)((u8*)D_800B235C + 0x14)', '(u32)*(u16*)(uintptr_t)(*(u32*)((u8*)D_800B235C + 0x14))'),
 'signed-shift':('(u32)*(u16*)((u8*)D_800B235C + 0x14)', '*(u16*)((u8*)D_800B235C + 0x14)'),
}
# Baseline source need not have the corrected call yet; compilation below is
# the initial failing ABI control. Mutant validity is checked after that gate.
for name,(old,new) in mutations.items():
 (out/(name+'.c')).write_text(body.replace(old,new,1))
 (out/(name+'.changed')).write_text(str(old in body))
PY
common=(-std=gnu17 -Wall -Wextra -Werror -DXENO_PC_PORT -Ipc_port/tests -Ipc_port/src)
for mode in O0 O2 UBSan; do
 flags=(-"$mode")
 if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 "$CC" "${common[@]}" "${flags[@]}" "$OUT/production.c" pc_port/tests/field_sound_events_retail_test.c \
   pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
 env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mode" disc/field.bin | tee "$OUT/$mode.log"
done
cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSan.log"
for mutant in channel bank-offset deadline narrow-state counter header-pointer signed-shift; do
 test "$(cat "$OUT/$mutant.changed")" = True
 "$CC" "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all "$OUT/$mutant.c" pc_port/tests/field_sound_events_retail_test.c \
   pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
 if env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mutant" disc/field.bin >"$OUT/$mutant.log" 2>&1; then
   echo "FIELD SOUND EVENTS mutant survived: $mutant" >&2; exit 1
 fi
 if [[ "$mutant" == header-pointer ]]; then
   grep -q 'runtime error: load of null pointer' "$OUT/$mutant.log"
 elif [[ "$mutant" == signed-shift ]]; then
   grep -q 'runtime error: left shift' "$OUT/$mutant.log"
 else
   grep -q 'FIELD SOUND EVENTS FAIL' "$OUT/$mutant.log"
 fi
done
echo 'FIELD SOUND EVENTS negative controls PASS: channel bank-offset deadline narrow-state counter header-pointer signed-shift'
echo "FIELD SOUND EVENTS receipts: $OUT"
