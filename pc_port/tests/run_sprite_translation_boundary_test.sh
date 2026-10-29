#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"
CC="${CC:-gcc}"
SAN_CC="${SAN_CC:-$(command -v clang || printf '%s' "$CC")}"
ubsan_preflight "$SAN_CC"
cd "$(dirname "$0")/../.."
OUT="${SPRITE_TRANSLATION_OUT:-pc_port/build_native/sprite_translation_boundary}"
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,os,sys
out=Path(sys.argv[1]);src=Path(os.environ.get('SPRITE_TRANSLATION_SOURCE','src/slus_006.64/system/temp1e.c'));s=src.read_text()
b=Path('disc/SLUS_006.64').read_bytes();assert sha256(b).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
h=sha256(b[0x15f18:0x15ff0]).hexdigest();assert h=='441bde165acd95177a764d12365c9656f1ec0f516fb055f8c80246979601a8ec'
a=s.index('void func_80025718(');z=s.index('\n}',a)+2
body='#include "sprite_translation_boundary_fixture.h"\n'+s[a:z]+'\n';(out/'production.c').write_text(body)
files=[src,Path('pc_port/tests/sprite_translation_boundary_fixture.h'),Path('pc_port/tests/sprite_translation_boundary_test.c'),Path('pc_port/tests/run_sprite_translation_boundary_test.sh'),Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/src/battle_mips_adapter.h')]
(out/'provenance.json').write_text(json.dumps({'image_sha256':sha256(b).hexdigest(),'function_sha256':h,'source_files':{str(p):sha256(p.read_bytes()).hexdigest() for p in files},'scope':'Extracted native prefix with fixed-width ABI fixture; pinned retail oracle stops before TransMatrix. No later branch, GTE or full-TU execution.'},indent=2)+'\n')
for name,old,new in [('narrow','    VECTOR trans;','    SVECTOR trans;'),('unsigned','trans.vx = *(s16*)','trans.vx = *(uint16_t*)'),('axis','pSprite + 0x06','pSprite + 0x02'),('matrix','pMatrix = pSub + 0x0C','pMatrix = pSub + 0x08')]:
 (out/(name+'.c')).write_text(body.replace(old,new,1));(out/(name+'.changed')).write_text(str(old in body))
PY
common=(-std=gnu17 -Wno-error=incompatible-pointer-types -DXENO_PC_PORT -Ipc_port/src -Ipc_port/tests -ffunction-sections -fdata-sections)
for mode in O0 O2 UBSan;do
 compiler="$CC"; flags=(-"$mode");if [[ "$mode" == UBSan ]];then compiler="$SAN_CC"; flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 "$compiler" "${common[@]}" "${flags[@]}" "$OUT/production.c" pc_port/tests/sprite_translation_boundary_test.c pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -o "$OUT/$mode" >"$OUT/$mode.build.log" 2>&1
 printf '%s\n' "$compiler" >"$OUT/$mode.compiler.txt"
 env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mode" | tee "$OUT/$mode.log"
done
cmp "$OUT/O0.log" "$OUT/O2.log";cmp "$OUT/O0.log" "$OUT/UBSan.log"
for mutant in narrow unsigned axis matrix;do
 test "$(cat "$OUT/$mutant.changed")" = True
 "$CC" "${common[@]}" -O0 "$OUT/$mutant.c" pc_port/tests/sprite_translation_boundary_test.c pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -o "$OUT/$mutant" >"$OUT/$mutant.build.log" 2>&1
 if env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1;then echo "SPRITE TRANSLATION mutant survived: $mutant" >&2;exit 1;fi
 grep -q 'SPRITE TRANSLATION FAIL' "$OUT/$mutant.log"
done
echo 'SPRITE TRANSLATION negative controls PASS: narrow unsigned axis matrix'
