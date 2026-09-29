#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "$PWD/pc_port/build_native/items-exit.XXXXXXXX")
SOURCE=${ITEMS_EXIT_SOURCE:-src/menu/main/misc.c}
echo "OUTPUT $OUT"
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
import sys,hashlib,json
s=Path(sys.argv[1]).read_text();out=Path(sys.argv[2])
a=s.index('void func_801DA518(void) {');b=s.index('\n#ifndef XENO_PC_PORT',a)
(out/'production.inc').write_text(s[a:b])
raw=Path('disc/menu.bin').read_bytes()[0x15518:0x155bc]
assert hashlib.sha256(raw).hexdigest()=='94b58bf69450701b2d2ce4efbe30785bd94ada49bbec6e6000bd207a4b065a32'
paths=[Path(sys.argv[1]),Path('include/system/menu.h'),Path('pc_port/tests/menu_items_exit_layout_test.c'),Path('pc_port/tests/run_menu_items_exit_layout_test.sh')]
(out/'pins.json').write_text(json.dumps({str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},indent=2)+'\n')
PY
COMMON=(-std=gnu17 -fno-pie -no-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode"); if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 clang "${COMMON[@]}" "${flags[@]}" pc_port/tests/menu_items_exit_layout_test.c -o "$OUT/$mode"
 "$OUT/$mode"
done
cp "$OUT/production.inc" "$OUT/good.inc"
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=(p/'good.inc').read_text()
controls=[('wrong-description','((ItemMenuWork*)(uintptr_t)g_Menu->unk42C[0])->descriptionBundle','*(u32*)((u8*)(uintptr_t)g_Menu->unk42C[0]+0x1180)'),('wide-description','((ItemMenuWork*)(uintptr_t)g_Menu->unk42C[0])->descriptionBundle','*(uintptr_t*)&((ItemMenuWork*)(uintptr_t)g_Menu->unk42C[0])->descriptionBundle'),('skip-work-free','HeapFree((void*)(uintptr_t)g_Menu->unk42C[0]);',''),('wrong-window','func_801D4EA0(4);','func_801D4EA0(5);'),('missing-disable','g_Menu->pManager->unk48 = 0;','')]
for name,old,new in controls:
 assert s.count(old)==1,(name,s.count(old));(p/(name+'.inc')).write_text(s.replace(old,new,1))
(p/'cached-menu.inc').write_text(s.replace('void func_801DA518(void) {','void func_801DA518(void) {\n SystemMenu *cached = g_Menu;').replace('g_Menu->','cached->'))
PY
for name in wrong-description wide-description skip-work-free wrong-window missing-disable cached-menu; do
 cp "$OUT/$name.inc" "$OUT/production.inc"
 clang "${COMMON[@]}" -O2 pc_port/tests/menu_items_exit_layout_test.c -o "$OUT/$name"
 if "$OUT/$name" > "$OUT/$name.log" 2>&1; then echo "ERROR mutant survived: $name"; exit 1; fi
 echo "REJECTED $name"
done
cp "$OUT/good.inc" "$OUT/production.inc"
