#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "$PWD/pc_port/build_native/scroll-handle.XXXXXXXX")
echo "OUTPUT $OUT"
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys,hashlib,json,re
p=Path(sys.argv[1]);s=Path('src/menu/main/misc.c').read_text();a=s.index('void func_801D1464(void) {');b=s.index('\nvoid func_801D14B0',a);(p/'production.inc').write_text(s[a:b])
bin=Path('disc/menu.bin').read_bytes();asm=Path('asm/menu/main/misc.s').read_text().split('glabel func_801D1464\n')[1].split('.size func_801D1464')[0]
assert hashlib.sha256(bin).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
words=re.findall(r'/\* ([0-9A-F]+) ([0-9A-F]+) ([0-9A-F]{8}) \*/',asm)
assert len(words)==19
for off,addr,w in words:assert bin[int(off,16):int(off,16)+4]==bytes.fromhex(w)
paths=[Path('src/menu/main/misc.c'),Path('include/system/menu.h'),Path('pc_port/tests/menu_scroll_handle_layout_test.c'),Path('pc_port/tests/run_menu_scroll_handle_layout_test.sh')]
(p/'pins.json').write_text(json.dumps({str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in paths},indent=2))
PY
COMMON=(-std=gnu17 -fno-pie -no-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode");if [ "$mode" = UBSan ];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 clang "${COMMON[@]}" "${flags[@]}" pc_port/tests/menu_scroll_handle_layout_test.c -o "$OUT/$mode"
 "$OUT/$mode"
done
cp "$OUT/production.inc" "$OUT/good.inc"
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=(p/'good.inc').read_text()
for name,old,new in [('only-one','scrollHandleActive != 0','scrollHandleActive == 1'),('wrong-context','handle->renderContext','0'),('wrong-vertices','handle->vertices','handle->polys'),('wrong-count','func_801CE198(1,','func_801CE198(2,')]:
 assert s.count(old)==1;(p/(name+'.inc')).write_text(s.replace(old,new,1))
PY
for name in only-one wrong-context wrong-vertices wrong-count; do
 cp "$OUT/$name.inc" "$OUT/production.inc"
 clang "${COMMON[@]}" -O2 pc_port/tests/menu_scroll_handle_layout_test.c -o "$OUT/$name"
 if "$OUT/$name" > "$OUT/$name.log" 2>&1; then echo "ERROR survived $name";exit 1;fi
 echo "REJECTED $name"
done
cp "$OUT/good.inc" "$OUT/production.inc"
