#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"
CC="${CC:-gcc}"
ubsan_preflight "$CC"
cd "$(dirname "$0")/../.."
OUT="${SOUND_PAIR_PITCH_OUT:-pc_port/build_native/sound_pair_pitch}"
mkdir -p "$OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys,os
out=Path(sys.argv[1]);src=Path('src/slus_006.64/system/sound.c');s=src.read_text();b=Path('disc/SLUS_006.64').read_bytes()
assert sha256(b).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
h=sha256(b[0x2ac50:0x2acfc]).hexdigest();assert h=='d4933cadc8fcbf9802872ab301c26264c02b419591b252873371c20fefcae365'
data_h=sha256(b[0x2abb8:0x2ac50]).hexdigest();assert data_h=='c7bad0d61846611afeca7e67f4762dd3ac3c5d64500f70747b2f11425aaa7480'
a=s.index('void func_8003A450(');z=s.index('\n}',a)+2
da=s.index('void func_8003A3B8(');dz=s.index('\n}',da)+2
body='#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16; typedef uint32_t u32; typedef int32_t s32;\nextern u32 D_800595D8; extern s32 D_80059478;\n'+s[a:z]+'\n'+s[da:dz]+'\n'
(out/'production.c').write_text(body)
(out/'provenance.json').write_text(json.dumps({'mode':'sound-tu-with-section-gc' if os.environ.get('SOUND_PAIR_FULL_TU')=='1' else 'extracted','image_sha256':sha256(b).hexdigest(),'function_sha256':h,'data_function_sha256':data_h,'extracted_sha256':sha256(body.encode()).hexdigest(),'scope':'Default verbatim extracted function, fixture declarations. Optional SOUND_PAIR_FULL_TU=1 compiles sound.c with section GC. Neither is a full game/audio test.','source_files':{str(p):sha256(p.read_bytes()).hexdigest() for p in [src,Path('pc_port/tests/sound_pair_pitch_retail_test.c'),Path('pc_port/tests/run_sound_pair_pitch_retail_test.sh'),Path('pc_port/src/battle_mips_adapter.c')]}},indent=2)+'\n')
for name,old,new in [('stride','idx * 0x158','idx * 0x2B8'),('narrow','s32 pitchWord = (s32)((u32)pitch << 8);','s32 pitchWord = (u16)((u32)pitch << 8);'),('signed','(u32)pitch << 8','pitch << 8'),('count','s32 count = 2;','s32 count = 1;')]:
 (out/(name+'.c')).write_text(body.replace(old,new,1));(out/(name+'.changed')).write_text(str(old in body))
for name,old,new in [('data-narrow','s32 pitchWord = (s32)((u32)pitch << 8);','s32 pitchWord = (u16)((u32)pitch << 8);'),('data-filter','== pData','!= pData'),('data-count','s32 count = D_80059478;','s32 count = D_80059478 - 1;')]:
 data_body=s[da:dz];native_data=data_body.split('#else',1)[0]
 (out/(name+'.changed')).write_text(str(old in native_data))
 changed_data=native_data.replace(old,new,1)+data_body[len(native_data):]
 (out/(name+'.c')).write_text(body.replace(data_body,changed_data,1))
PY
common=(-std=gnu17 -fno-pie -no-pie -DXENO_PC_PORT -Ipc_port/src -ffunction-sections -fdata-sections)
source_file="$OUT/production.c"
extra=()
if [[ "${SOUND_PAIR_FULL_TU:-0}" == 1 ]];then
 source_file=src/slus_006.64/system/sound.c
 extra=(-DSKIP_ASM -D_LANGUAGE_C -include assert.h -fpermissive -w -Iinclude -Ipc_port/include)
fi
for mode in O0 O2 UBSan;do
 flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 "$CC" "${common[@]}" "${extra[@]}" "${flags[@]}" "$source_file" pc_port/tests/sound_pair_pitch_retail_test.c pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -lm -o "$OUT/$mode"
 env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mode" | tee "$OUT/$mode.log"
done
cmp "$OUT/O0.log" "$OUT/O2.log";cmp "$OUT/O0.log" "$OUT/UBSan.log"
for mutant in stride narrow signed count data-narrow data-filter data-count;do
 test "$(cat "$OUT/$mutant.changed")" = True
 "$CC" "${common[@]}" -O1 -fsanitize=undefined -fno-sanitize-recover=all "$OUT/$mutant.c" pc_port/tests/sound_pair_pitch_retail_test.c pc_port/src/battle_mips_adapter.c -Wl,--gc-sections -o "$OUT/$mutant"
 if env -u XENO_DISABLE_CPU_INTERPRETER "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1;then echo "PAIR PITCH mutant survived: $mutant" >&2;exit 1;fi
 if [[ "$mutant" == signed ]];then grep -q 'runtime error: left shift' "$OUT/$mutant.log";else grep -Eq '(PAIR|DATA) PITCH FAIL' "$OUT/$mutant.log";fi
done
echo 'PAIR PITCH negative controls PASS: stride narrow signed count data-narrow data-filter data-count'
