#!/usr/bin/env bash
# func_801D7CFC (Gear screen ride glyphs): the port body against expected
# values hand-derived from the retail listing.  No retail code runs; see the
# header of pc_port/tests/menu_ride_glyphs_test.c.  The script first checks
# the listing's instruction words against the user's disc/menu.bin.  It then
# builds and runs O0, O2 and UBSan, and requires four mutants (the former
# port body's faults) to fail.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_ride_glyphs
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import hashlib, re
b=Path('disc/menu.bin').read_bytes()
words=re.findall(r'/\* \w+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/',next(Path('asm/menu').glob('*matchings/main/misc*/func_801D7CFC.s')).read_text())
for addr,h in words:
 off=int(addr,16)-0x801c5000
 assert b[off:off+4]==bytes.fromhex(h), (addr,h)
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
print('7CFC instruction words OK, bytes', len(words)*4)
s=Path('src/menu/main/misc.c').read_text()
a=s.index('void func_801D7CFC(s32 arg0, s32 arg1, s32 arg2) {')
z=s.index('\n}\n', a) + 3
hdr=('#include "common.h"\n#include "system/menu.h"\nextern s32 D_801E977C[];\n'
     'u8* xeno_raw_358(u32 offset);\n#define MenuRawPointer(o) xeno_raw_358(o)\n'
     'extern s32 func_8002675C(u8*, s32, void*, s32, s32, s32, s32);\n'
     'extern void func_801C851C(SVECTOR* verts, s32 x, s32 y, s32 w, s32 h);\n'
     'extern void func_801E91C4(POLY_FT4* p);\n')
b=s[a:z]
p=Path('pc_port/build_native/menu_ride_glyphs')
(p/'body.c').write_text(hdr+b)
mutants={
 'raw_render_context':('g_Menu->renderContext, 0x78 + i * 0x20','*(s32*)((u8*)g_Menu + 0x308), 0x78 + i * 0x20'),
 'loop_count':('for (i = 0; i < 2; i++)','for (i = 0; i < (u8)arg1; i++)'),
 'vertex_base':('(SVECTOR*)(data + 0x2AA0 + i * 0x20)','(SVECTOR*)((u8*)g_Menu + 0x78 + i * 0x20)'),
 'highlight_gate':('    func_801E91C4((POLY_FT4*)poly);','    if ((u8)arg1 >= 2) func_801E91C4((POLY_FT4*)poly);'),
}
for n,(old,new) in mutants.items():
 assert b.count(old)==1, (n,old,b.count(old))
 (p/(n+'.c')).write_text(hdr+b.replace(old,new))
PYGEN
BASE=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -fpermissive -w -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan; do
  flags=(-"$mode"); if [[ "$mode" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
  gcc "${BASE[@]}" "${flags[@]}" -c "$OUT/body.c" -o "$OUT/$mode.body.o"
  gcc "${BASE[@]}" "${flags[@]}" -c pc_port/tests/menu_ride_glyphs_test.c -o "$OUT/$mode.test.o"
  clang -no-pie "${flags[@]}" "$OUT/$mode.body.o" "$OUT/$mode.test.o" -o "$OUT/$mode"
  "$OUT/$mode" | tee "$OUT/$mode.log"
done
for mutant in raw_render_context loop_count vertex_base highlight_gate; do
  gcc "${BASE[@]}" -O2 -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
  clang -no-pie -O2 "$OUT/$mutant.o" "$OUT/O2.test.o" -o "$OUT/$mutant"
  if "$OUT/$mutant" >"$OUT/$mutant.log" 2>&1; then echo "FAIL survived $mutant"; exit 1; fi
  grep -q '^FAIL ride glyphs' "$OUT/$mutant.log"
  echo "MENU RIDE GLYPHS mutant rejected: $mutant ($(grep -m1 '^FAIL ride glyphs' "$OUT/$mutant.log" | cut -c1-90))"
done
echo 'MENU RIDE GLYPHS negative controls PASS count=4'
