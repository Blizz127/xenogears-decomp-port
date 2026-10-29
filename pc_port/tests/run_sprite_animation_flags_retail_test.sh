#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"
CC="${CC:-gcc}"
ubsan_preflight "$CC"
cd "$(dirname "$0")/../.."
OUT="${SPRITE_FLAGS_OUT:-pc_port/build_native/sprite_animation_flags}"
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,os,sys
out=Path(sys.argv[1]);src=Path(os.environ.get('SPRITE_FLAGS_SOURCE','src/slus_006.64/system/temp1.c'))
s=src.read_text();image=Path('disc/SLUS_006.64').read_bytes()
assert sha256(image).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
h=sha256(image[0x13970:0x139e0]).hexdigest()
assert h=='436ad1158c299e3b6bd78aa800a483f8c057386d66596fbaaf7330f2552ea4b2'
a=s.index('void func_80023170(');z=s.index('\n}',a)+2
body='#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16; typedef uint32_t u32; typedef int32_t s32;\nextern void func_8001D2B0(void*,s16);\n'+s[a:z]+'\n'
(out/'production.c').write_text(body)
(out/'provenance.json').write_text(json.dumps({'image_sha256':sha256(image).hexdigest(),'function_sha256':h,'test_files':{str(p):sha256(p.read_bytes()).hexdigest() for p in [Path('pc_port/tests/run_sprite_animation_flags_retail_test.sh'),Path('pc_port/tests/sprite_animation_flags_retail_test.c'),Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/src/battle_mips_adapter.h')]},'source':str(src),'source_sha256':sha256(s.encode()).hexdigest(),'extracted_sha256':sha256(body.encode()).hexdigest(),'scope':'Extracted native function versus pinned retail CPU oracle. Callee is observed and stubbed on both sides; no full TU or gameplay execution.'},indent=2)+'\n')
for name,old,new in [('bit5','& ~0x10','& ~0x30'),('frame','pSpriteData, animFrame','pSpriteData, 0'),('counter','*(u16*)(pData + 0x9E) = 0;','*(u16*)(pData + 0x9E) = 1;'),('maskA8','& 0xFFF1FFFF','& 0xFFFFFFFF')]:
 (out/(name+'.c')).write_text(body.replace(old,new,1))
 (out/(name+'.changed')).write_text(str(old in body))
PY
common=(-std=gnu17 -DXENO_PC_PORT -Ipc_port/src -ffunction-sections -fdata-sections)
for mode in O0 O2 UBSan;do
 flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 "$CC" "${common[@]}" "${flags[@]}" "$OUT/production.c" pc_port/tests/sprite_animation_flags_retail_test.c pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -o "$OUT/$mode"
 env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mode" | tee "$OUT/$mode.log"
done
cmp "$OUT/O0.log" "$OUT/O2.log";cmp "$OUT/O0.log" "$OUT/UBSan.log"
for mutant in bit5 frame counter maskA8;do
 test "$(cat "$OUT/$mutant.changed")" = True
 "$CC" "${common[@]}" -O2 "$OUT/$mutant.c" pc_port/tests/sprite_animation_flags_retail_test.c pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -o "$OUT/$mutant"
 if env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1;then echo "SPRITE FLAGS mutant survived: $mutant" >&2;exit 1;fi
 grep -q 'SPRITE FLAGS FAIL' "$OUT/$mutant.log"
done
echo 'SPRITE FLAGS negative controls PASS: bit5 frame counter maskA8'
