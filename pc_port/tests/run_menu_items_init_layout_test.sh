#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "$PWD/pc_port/build_native/items-init.XXXXXXXX")
SOURCE=${ITEMS_INIT_SOURCE:-src/menu/main/misc.c}
echo "OUTPUT $OUT"
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
import sys,hashlib,json
s=Path(sys.argv[1]).read_text();out=Path(sys.argv[2])
a=s.index('void func_801DA4A8(void) {');b=s.index('\nvoid func_801DA518',a)
(out/'production.inc').write_text(s[a:b])
raw=Path('disc/menu.bin').read_bytes()[0x154a8:0x15518]
assert hashlib.sha256(raw).hexdigest()=='06aab1a7db7c225166b03ac059f45cdcfeec49fd9e7180c9918cdf6ffde33226'
paths=[Path(sys.argv[1]),Path('include/system/menu.h'),Path('pc_port/tests/menu_items_init_layout_test.c'),Path('pc_port/tests/run_menu_items_init_layout_test.sh')]
(out/'pins.json').write_text(json.dumps({str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},indent=2)+'\n')
PY
COMMON=(-std=gnu17 -fno-pie -no-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode"); if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 clang "${COMMON[@]}" "${flags[@]}" pc_port/tests/menu_items_init_layout_test.c -o "$OUT/$mode"
 "$OUT/$mode"
done
cp "$OUT/production.inc" "$OUT/good.inc"
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=(p/'good.inc').read_text()
controls=[('string-offset','g_Menu->itemMenuStrings','(u8*)g_Menu + 0x10E0'),('slot-offset','g_Menu->unk42C[0] = (u32)(uintptr_t)work;','*(void**)((u8*)g_Menu + 0x42C) = work;'),('allocation-size','HeapAlloc(sizeof(ItemMenuWork), 0)','HeapAlloc(0x1198, 0)'),('clear-size','bzero(work, sizeof(ItemMenuWork))','bzero(work, 0x1198)')]
for name,old,new in controls:
 assert s.count(old)==1,(name,s.count(old))
 (p/(name+'.inc')).write_text(s.replace(old,new,1))
PY
for name in string-offset slot-offset allocation-size clear-size; do
 cp "$OUT/$name.inc" "$OUT/production.inc"
 clang "${COMMON[@]}" -O2 pc_port/tests/menu_items_init_layout_test.c -o "$OUT/$name"
 if "$OUT/$name" > "$OUT/$name.log" 2>&1; then echo "ERROR mutant survived: $name"; exit 1; fi
 echo "REJECTED $name"
done
cp "$OUT/good.inc" "$OUT/production.inc"
