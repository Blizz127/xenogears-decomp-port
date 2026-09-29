#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_dispatch_data_retail_test
scope=all
if [ "$#" -eq 1 ] && [ "$1" = --b5 ]; then
 scope=b5
 OUT="$OUT/b5_alias"
elif [ "$#" -ne 0 ]; then
 echo "usage: $0 [--b5]" >&2; exit 2
fi
mkdir -p "$OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
b=Path('disc/SLUS_006.64').read_bytes()
for a,z,h in [(0x800183d8,0x800185a4,'ee73be97683cbf2affa5db0d128e5083ac4b273da3bc5c2f0bfc12bfb75ee0f8'),(0x8001fbe4,0x80021ad8,'7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c')]:
 assert sha256(b[a-0x8000f800:z-0x8000f800]).hexdigest()==h
assert sha256(b[0x80022000-0x8000f800:0x80022038-0x8000f800]).hexdigest()=='daa73a955905fe34d31204c01207f55d4da7dadd4419a5437ba90afcd23d70db'
assert sha256(b[0x80021318-0x8000f800:0x80021340-0x8000f800]).hexdigest()=='99da33f91badd4b1f9b7af0266b3d94849c16872c0bf589b80caeab77197c1c8'
print('SPRITE DATA retail dispatch, B5 handler and scale helper pins PASS')
PY
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
 # Match the existing port's permissive compilation of this legacy TU;
 # warnings remain visible. The new oracle test and CPU compile with Werror.
 gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c src/slus_006.64/system/animation_scripts.c -o "$OUT/$opt.body.o"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_data_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" -o "$OUT/$opt.test"
 if [ "$scope" = b5 ]; then
  "$OUT/$opt.test" b5
  echo "SPRITE DATA focused B5 $opt PASS"
  continue
 fi
 "$OUT/$opt.test"
 "$OUT/$opt.test" b5
 if "$OUT/$opt.test" unhandled > "$OUT/$opt.unhandled.log" 2>&1; then
  echo 'SPRITE DATA unhandled instruction did not abort' >&2; exit 1
 fi
 python3 - "$OUT/$opt.unhandled.log" <<'PY'
import json,sys
lines=open(sys.argv[1]).read().splitlines()
events=[json.loads(line) for line in lines if line.startswith('{')]
assert len(events)==1
event=events[0]
assert event['event']=='sprite_animation_unimplemented'
assert event['raw_opcode']==0x12340090 and event['opcode']==0x90 and event['dispatch_index']==0x06
assert set(event)=={'event','raw_opcode','opcode','dispatch_index','sprite','operands'}
assert any('func_8001FBE4 dispatch path is not implemented' in line for line in lines)
PY
done
if [ "$scope" = all ]; then
for mutant in reset mask mirror add axis signed_byte relative position rounding; do
 case "$mutant" in
  reset) expression='s/AnimationWrite32(p + 0x14, 0)/AnimationWrite32(p + 0x10, 0)/' ;;
  mask) expression='s/0xFFFFF801u/0xFFFFF803u/' ;;
  mirror) expression='s/AnimationRead32(p + 0xAC) \& 4/AnimationRead32(p + 0xAC) \& 8/' ;;
  add) expression='s/== 0xAE) value/== 0xAF) value/' ;;
  axis) expression='s/== 0xB6 ? 0 : 2/== 0xB6 ? 2 : 0/' ;;
  signed_byte) expression='s/(s8)((u8\*)operands)\[0\] \* 16/(u8)((u8*)operands)[0] * 16/' ;;
  relative) expression='s/s32 relative = (s16)AnimationRead16/s32 relative = (u16)AnimationRead16/' ;;
  position) expression='s/value << 16);/value << 15);/' ;;
  rounding) expression='s/if (product < 0) product += 0xFFF;/if (product < 0) product += 0;/' ;;
 esac
 sed "$expression" src/slus_006.64/system/animation_scripts.c > "$OUT/$mutant.c"
 gcc -std=gnu17 -fpermissive -O2 -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -std=c17 -O2 -fno-pie -no-pie -Ipc_port/src -Wl,--gc-sections pc_port/tests/sprite_dispatch_data_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "SPRITE DATA mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'SPRITE DATA FAIL' "$OUT/$mutant.log"
done
echo 'SPRITE DATA negative controls PASS: reset mask mirror add axis signed_byte relative position rounding'
fi
for mutant in b5_gate b5_scale b5_dirty b5_dirty_order; do
 python3 - "$mutant" "$OUT/$mutant.c" <<'PY'
from pathlib import Path
import sys

changes = {
 'b5_gate': ('if (flags & 3u) SpriteSetScale', 'if (flags & 1u) SpriteSetScale'),
 'b5_scale': ('s16 scale = (s16)((s8)((u8*)operands)[0] * 256);',
              's16 scale = (s16)((s8)((u8*)operands)[0] * 128);'),
 'b5_dirty': ('AnimationWrite32(pData + 0x3C, AnimationRead32(pData + 0x3C) | 0x10000000u);',
              'AnimationWrite32(pData + 0x3C, AnimationRead32(pData + 0x3C) | 0x20000000u);'),
 'b5_dirty_order': (
     '        AnimationWrite16(pData + 0x2C, (u16)scale);\n'
     '        AnimationWrite16(pBase + 0xA, (u16)scale);\n'
     '        AnimationWrite16(pBase + 0x8, (u16)scale);\n'
     '        AnimationWrite16(pBase + 0x6, (u16)scale);\n'
     '        AnimationWrite32(pData + 0x3C, AnimationRead32(pData + 0x3C) | 0x10000000u);',
     '        AnimationWrite32(pData + 0x3C, AnimationRead32(pData + 0x3C) | 0x10000000u);\n'
     '        AnimationWrite16(pData + 0x2C, (u16)scale);\n'
     '        AnimationWrite16(pBase + 0xA, (u16)scale);\n'
     '        AnimationWrite16(pBase + 0x8, (u16)scale);\n'
     '        AnimationWrite16(pBase + 0x6, (u16)scale);'),
}
source = Path('src/slus_006.64/system/animation_scripts.c').read_text()
before, after = changes[sys.argv[1]]
assert source.count(before) == 1, (sys.argv[1], source.count(before))
Path(sys.argv[2]).write_text(source.replace(before, after))
PY
 gcc -std=gnu17 -fpermissive -O2 -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -std=c17 -O2 -fno-pie -no-pie -Ipc_port/src -Wl,--gc-sections pc_port/tests/sprite_dispatch_data_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" b5 > "$OUT/$mutant.log" 2>&1; then
  echo "SPRITE DATA mutant survived: $mutant" >&2; exit 1
 fi
 rg -q 'SPRITE DATA FAIL' "$OUT/$mutant.log"
 echo "SPRITE DATA B5 negative control PASS: $mutant"
done
echo 'SPRITE DATA B5 negative controls PASS: gate scale dirty dirty_order'
