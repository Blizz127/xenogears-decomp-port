#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
SOURCE=${SYSTEM_MENU_FRAME_SOURCE:-src/slus_006.64/system/menu.c}
OUT=${SYSTEM_MENU_FRAME_OUT:-$(mktemp -d /tmp/xeno-system-menu-frame.XXXXXXXX)}
mkdir -p "$OUT"
echo "SYSTEM MENU FRAME OUTPUT $OUT"
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
source=Path(sys.argv[1]);out=Path(sys.argv[2]);disc=Path('disc/SLUS_006.64')
assert sha256(disc.read_bytes()).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
paths=[source,disc,Path('pc_port/tests/system_menu_frame_retail_test.c'),Path('pc_port/tests/run_system_menu_frame_retail_test.sh'),Path('pc_port/src/battle_mips_adapter.c'),Path('pc_port/src/battle_mips_adapter.h'),Path('include/system/menu.h'),Path('pc_port/extern/PsyCross/include/PsyX/PsyX_config.h'),Path('pc_port/build_port.sh')]
(out/'pins.json').write_text(json.dumps({'source_pins':{str(p.resolve()):sha256(p.read_bytes()).hexdigest() for p in paths},'scope':'actual full production TU menu frame against 308 retail bytes; boundary spies, not GPU rendering'},indent=2)+'\n')
PY
COMMON=(-include assert.h -std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in ${SYSTEM_MENU_FRAME_MODES:-O0 O2 UBSan}; do
 flags=(-"$mode");if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c "$SOURCE" -o "$OUT/$mode.source.o"
 objcopy --weaken-symbol=MenuProcessControllerInput "$OUT/$mode.source.o"
 gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/system_menu_frame_retail_test.c -o "$OUT/$mode.test.o"
 clang "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
 "$OUT/$mode" disc/SLUS_006.64 > "$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log";exit 1; }
 cat "$OUT/$mode.log"
done
if [ "${SYSTEM_MENU_FRAME_SKIP_CONTROLS:-0}" != 1 ]; then
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
import json,sys
s=Path(sys.argv[1]).read_text();out=Path(sys.argv[2]);a=s.index('void func_8001C074(void) {');b=s.index('\nextern char D_8001833C',a);body=s[a:b]
controls={}
for name,old,new in (
 ('stuck-buffer','env = &g_Menu->gfxEnvs[0];','env = g_Menu->pGfxEnv;'),
 ('wrong-submit-entry','DrawOTag(&g_Menu->pGfxEnv->ot[15]);','DrawOTag(&g_Menu->pGfxEnv->ot[14]);'),
 ('wrong-page-toggle','g_Menu->renderContext == 0','g_Menu->renderContext ^ 1'),
 ('skip-debug-recheck','if (*D_8005917C != -1) {\n            FontDrawLetters','if (1) {\n            FontDrawLetters'),
):
 assert body.count(old)==1,(name,body.count(old))
 file=out/(name+'.c');file.write_text(s[:a]+body.replace(old,new,1)+s[b:]);controls[name]=str(file)
(out/'controls.json').write_text(json.dumps(controls,indent=2)+'\n')
PY
for control in stuck-buffer wrong-submit-entry wrong-page-toggle skip-debug-recheck; do
 gcc "${COMMON[@]}" -O2 -fpermissive -w -c "$OUT/$control.c" -o "$OUT/$control.source.o"
 objcopy --weaken-symbol=MenuProcessControllerInput "$OUT/$control.source.o"
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$control.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/$control"
 rc=0
 "$OUT/$control" disc/SLUS_006.64 > "$OUT/$control.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! rg -q '^SYSTEM MENU FRAME FAIL ' "$OUT/$control.log"; then
  cat "$OUT/$control.log";echo "SYSTEM MENU FRAME control failed to detect defect: $control rc=$rc" >&2;exit 1
 fi
 echo "SYSTEM MENU FRAME control rejected: $control"
done
fi
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
p=json.loads((Path(sys.argv[1])/'pins.json').read_text())
for path,wanted in p['source_pins'].items():assert sha256(Path(path).read_bytes()).hexdigest()==wanted,path
PY
