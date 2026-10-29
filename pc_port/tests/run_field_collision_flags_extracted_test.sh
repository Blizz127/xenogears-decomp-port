#!/usr/bin/env bash
# Explicit dependency-light boundary test; full-TU runner remains separate.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"
CC="${CC:-gcc}"
ubsan_preflight "$CC"
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT="${COLLISION_FLAGS_OUT:-pc_port/build_native/collision_flags_extracted}"
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
out=Path(sys.argv[1]);src=Path('src/field/main/misc11.c');s=src.read_text()
b=Path('disc/field.bin').read_bytes()
assert sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
ranges=[(0x80092424,0x800924d4,'b6ec77629f5f3aadac9084334163fb0377430ce8d33ea3a38d53999f819df367'),(0x800924d4,0x800925a0,'c125f26e7774584bd3b9287e5b45c278f3c5f3d978de82ed30f925e580d25d2d')]
for a,z,h in ranges:assert sha256(b[a-0x8006faf0:z-0x8006faf0]).hexdigest()==h
start=s.rindex('#ifdef XENO_PC_PORT',0,s.index('u32 func_80092424('))
a=s.index('void func_800924D4(');end=s.index('\n}',a)+2
body='#include <stdint.h>\ntypedef uint8_t u8; typedef uint32_t u32; typedef int32_t s32;\n'+s[start:end]+'\n'
(out/'production.c').write_text(body)
(out/'provenance.json').write_text(json.dumps({'field_sha256':sha256(b).hexdigest(),'function_ranges':ranges,'extracted_source_sha256':sha256(body.encode()).hexdigest(),'scope':'Verbatim getter/writer plus production pointer declarations/macro. Extracted boundary only, not production-TU linkage or gameplay. Reference executes test-only CPU adapter.','source_files':{str(p):sha256(p.read_bytes()).hexdigest() for p in [src,Path('pc_port/tests/field_collision_flags_test.c'),Path('pc_port/tests/run_field_collision_flags_extracted_test.sh'),Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/src/battle_mips_adapter.h')]}},indent=2)+'\n')
mutations={
 'mask-input':('    u32* entry;', '    value = (u32)value & 0xFFu;\n    u32* entry;'),
 'wrong-shift':('((u32)value << 8)', '((u32)value << 16)'),
 'signed-shift':('((u32)value << 8)', '(value << 8)'),
}
for name,(old,new) in mutations.items():
 (out/(name+'.c')).write_text(body.replace(old,new,1));(out/(name+'.changed')).write_text(str(old in body))
PY
common=(-std=gnu17 -fno-pie -no-pie -fno-builtin -DXENO_PC_PORT -Ipc_port/src)
for mode in O0 O2 UBSan; do
 flags=(-"$mode")
 if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
 "$CC" "${common[@]}" "${flags[@]}" "$OUT/production.c" pc_port/tests/field_collision_flags_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
 env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mode" | tee "$OUT/$mode.log"
done
cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSan.log"
for mutant in mask-input wrong-shift signed-shift; do
 test "$(cat "$OUT/$mutant.changed")" = True
 "$CC" "${common[@]}" -O2 -fsanitize=undefined -fno-sanitize-recover=all "$OUT/$mutant.c" pc_port/tests/field_collision_flags_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
 if env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1; then
   echo "COLLISION FLAGS mutant survived: $mutant" >&2; exit 1
 fi
 if [[ "$mutant" == signed-shift ]]; then grep -q 'runtime error: left shift' "$OUT/$mutant.log";
 else grep -q 'COLLISION FLAGS FAIL' "$OUT/$mutant.log"; fi
done
echo 'COLLISION FLAGS extracted negative controls PASS: mask-input wrong-shift signed-shift'
echo "COLLISION FLAGS extracted receipts: $OUT"
