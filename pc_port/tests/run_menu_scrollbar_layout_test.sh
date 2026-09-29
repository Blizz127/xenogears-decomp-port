#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "$PWD/pc_port/build_native/scrollbar.XXXXXXXX")
echo "OUTPUT $OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys,json,hashlib
assert hashlib.sha256(Path('disc/menu.bin').read_bytes()[0xec4c:0xedb0]).hexdigest()=='74d828b10b7dcf9abce251f6bc73494794845f9828296af73c30afed3ec551bc'
p=Path(sys.argv[1]);s=Path('src/menu/main/misc.c').read_text();a=s.index('void func_801D3C4C(');b=s.index('\n}\n',a)+3;body=s[a:b];(p/'production.inc').write_text(body)
assert 'func_801D3C4C(windowIndex, x, y, w, h);' in s
paths=[Path('src/menu/main/misc.c'),Path('include/system/menu.h'),Path('pc_port/tests/menu_scrollbar_layout_test.c'),Path('pc_port/tests/run_menu_scrollbar_layout_test.sh')]
(p/'pins.json').write_text(json.dumps({str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in paths},indent=2))
PY
COMMON=(-std=gnu17 -fno-pie -no-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode");if [ "$mode" = UBSan ];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 clang "${COMMON[@]}" "${flags[@]}" pc_port/tests/menu_scrollbar_layout_test.c -o "$OUT/$mode"
 "$OUT/$mode"
done
cp "$OUT/production.inc" "$OUT/good.inc"
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys,hashlib
assert hashlib.sha256(Path('disc/menu.bin').read_bytes()[0xec4c:0xedb0]).hexdigest()=='74d828b10b7dcf9abce251f6bc73494794845f9828296af73c30afed3ec551bc'
p=Path(sys.argv[1]);s=(p/'good.inc').read_text()
for name,old,new in [('width-as-height','(void)w;','h = w;'),('middle-height','(u16)(h - 8)','(u16)h'),('signed-bottom','8, 0xFFF8','8, -8'),('missing-wrap','(u16)(y16 + h)','(y16 + h)'),('wrong-cap','polysScrollBarEnds[2]','polysScrollBarEnds[0]')]:
 assert s.count(old)==1;(p/(name+'.inc')).write_text(s.replace(old,new,1))
(p/'cached-menu.inc').write_text(s.replace('MenuWindow* window =','SystemMenu* cached = g_Menu;\n    MenuWindow* window =',1).replace('g_Menu->','cached->'))
PY
for name in width-as-height middle-height signed-bottom missing-wrap wrong-cap cached-menu; do
 cp "$OUT/$name.inc" "$OUT/production.inc"
 clang "${COMMON[@]}" -O2 pc_port/tests/menu_scrollbar_layout_test.c -o "$OUT/$name"
 if "$OUT/$name" > "$OUT/$name.log" 2>&1; then echo "ERROR mutant survived $name";exit 1;fi
 echo "REJECTED $name"
done
cp "$OUT/good.inc" "$OUT/production.inc"
