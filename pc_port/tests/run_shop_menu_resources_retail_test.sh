#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
SCRIPT=$(realpath "$0")
cd "${SHOP_MENU_RESOURCES_ROOT:-$(dirname "$0")/../..}"
ulimit -c 0
SOURCE=${SHOP_MENU_RESOURCES_SOURCE:-src/shop_menu/main/misc.c}
TEST=${SHOP_MENU_RESOURCES_TEST_SOURCE:-pc_port/tests/shop_menu_resources_retail_test.c}
OUT=${SHOP_MENU_RESOURCES_OUT:-$(mktemp -d /tmp/xeno-shop-menu-resources.XXXXXXXX)}
echo "SHOP RESOURCES OUTPUT $OUT"
mkdir -p "$OUT"
python3 - "$SOURCE" "$TEST" "$SCRIPT" "$OUT" <<'PY_PINS'
from pathlib import Path
from hashlib import sha256
import json,sys
source=Path(sys.argv[1]);test=Path(sys.argv[2]);script=Path(sys.argv[3]);out=Path(sys.argv[4]);disc=Path('disc/shop_menu.bin');raw=disc.read_bytes()
assert sha256(raw).hexdigest()=='7890e14bcabddcf85368de10783ecff8daa166dc5ac254e06db5e27959cab4cf'
assert sha256(raw[0x4b4:0x8f4]).hexdigest()=='f791af75ac8be21876feb8f706399780e3010ab17b780e8337fd328dcbfb2d51'
assert raw[:16]==bytes.fromhex('4249534c50532d303038303000000300')
paths=[source,test,script,disc,Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/src/battle_mips_adapter.h'),Path('include/system/menu.h'),Path('include/types.h'),Path('include/common.h'),Path('pc_port/include_shim/psyq/libgpu.h'),Path('pc_port/extern/PsyCross/include/psx/libgpu.h')]
(out/'pins.json').write_text(json.dumps({'inputs':{str(x.resolve()):sha256(x.read_bytes()).hexdigest() for x in paths},'scope':'actual full shop source and exact C data; explicit helper boundaries, no actual decompression/CD/GPU/audio; high native objects with low packed resource table'},indent=2)+'\n')
(out/'test-prototypes.h').write_text('/* Unrelated later full-TU definition; no tested body rewritten. */\nvoid func_801D1F10(void);\n')
PY_PINS
COMMON=(-include assert.h -include "$OUT/test-prototypes.h" -std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan;do
 flags=(-"$mode");if [ "$mode" = UBSan ];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 clang "${COMMON[@]}" "${flags[@]}" -Wno-everything -c "$SOURCE" -o "$OUT/$mode.source.o"
 clang "${COMMON[@]}" "${flags[@]}" -c "$TEST" -o "$OUT/$mode.test.o"
 clang "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
 "$OUT/$mode" disc/shop_menu.bin > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log";exit 1; }
 cat "$OUT/$mode.log"
done
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
import json,sys
p=Path(sys.argv[2]);s=Path(sys.argv[1]).read_text();a=s.index('void ShopMenuLoadResources(void)');b=s.index('\nvoid ShopMenuInitialize(void)',a);body=s[a:b];controls={}
for name,old,new in [('short-filename','D_801C5000, 13','D_801C5000, 12'),('packed-pointer-width','u32* pResources;','void** pResources;'),('wrong-portrait-stride','characterID * 0xB20','characterID * 0xB1C'),('missing-twelve-pixels','+ 12;','+ 0;'),('skip-valid-character','if (characterID != 0xFF)','if (characterID != 0xFF && characterID != 0)'),('reload-resource-global','pResources[7]','((u32*)D_8005945C)[7]')]:
 assert body.count(old)==1,(name,body.count(old));f=p/(name+'.c');f.write_text(s[:a]+body.replace(old,new,1)+s[b:]);controls[name]=str(f)
# Preserve function-local alias deliberately to detect stale g_Menu caching.
f=p/'cached-menu.c';f.write_text(s[:a]+body.replace('void ShopMenuLoadResources(void) {','void ShopMenuLoadResources(void) {\n    SystemMenu *cachedMenu = g_Menu;',1).replace('g_Menu->','cachedMenu->')+s[b:]);controls['cached-menu']=str(f)
(p/'controls.json').write_text(json.dumps(controls,indent=2)+'\n')
PY
for control in short-filename packed-pointer-width wrong-portrait-stride missing-twelve-pixels skip-valid-character reload-resource-global cached-menu;do
 clang "${COMMON[@]}" -O2 -Wno-everything -c "$OUT/$control.c" -o "$OUT/$control.source.o"
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$control.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/$control"
 rc=0
 "$OUT/$control" disc/shop_menu.bin > "$OUT/$control.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! rg -q '^SHOP RESOURCES FAIL ' "$OUT/$control.log";then cat "$OUT/$control.log";echo "CONTROL FAIL $control rc=$rc";exit 1;fi
 echo "SHOP RESOURCES CONTROL REJECTED $control"
done
python3 - "$OUT" <<'PY_VERIFY'
from pathlib import Path
from hashlib import sha256
import json,sys
pins=json.loads((Path(sys.argv[1])/'pins.json').read_text())
for path,wanted in pins['inputs'].items():assert sha256(Path(path).read_bytes()).hexdigest()==wanted,path
PY_VERIFY
