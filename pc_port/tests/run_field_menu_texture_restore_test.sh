#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_menu_texture_restore
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import re,hashlib
b=Path('disc/field.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
assert hashlib.sha256(b[0x8007a1d8-0x8006faf0:0x8007a21c-0x8006faf0]).hexdigest()=='4fe9eee5e7f887b2c5361c5b372b281ff342550406a0e7634887de66bc64dccc'
s=Path('src/field/main/misc4.c').read_text();a=s.index('    rect.x = 0x2C0;',s.index('    func_80070508();',s.index('void func_800799D4(void)')));z=s.index('    HeapFree(pSaveBottom);',a)
Path('pc_port/build_native/field_menu_texture_restore/texture_restore.inc').write_text('typedef unsigned int u_long;\nstatic void native_restore(void *pSaveBottom,void *pSaveTop){RECT rect;\n'+s[a:z]+'}\n')
PYGEN
for mode in O0 O2 UBSan; do
 flags=(-"$mode")
 if [[ "$mode" == UBSan ]];then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all);fi
 gcc -std=c17 "${flags[@]}" -I"$OUT" -Ipc_port/src -c pc_port/tests/field_menu_texture_restore_test.c -o "$OUT/$mode.test.o"
 gcc -std=c17 "${flags[@]}" -Ipc_port/src -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
 clang -no-pie "${flags[@]}" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
 "$OUT/$mode" | tee "$OUT/$mode.log"
done

python3 - <<'PYGEN'
from pathlib import Path
p=Path('pc_port/build_native/field_menu_texture_restore');s=(p/'texture_restore.inc').read_text()
mutants={'wrong_y':('rect.y = 0x100;','rect.y = 0;'),'wrong_x':('rect.x = 0x3C0;','rect.x = 0x380;'),'wrong_source':('(u_long*)pSaveBottom','(u_long*)pSaveTop'),'missing_second':('LoadImage(&rect, (u_long*)pSaveTop);',';')}
for n,(old,new) in mutants.items():
 assert s.count(old)==1
 (p/n).mkdir(exist_ok=True);(p/n/'texture_restore.inc').write_text(s.replace(old,new))
PYGEN
for mutant in wrong_y wrong_x wrong_source missing_second;do
 gcc -std=c17 -O2 -I"$OUT/$mutant" -Ipc_port/src -c pc_port/tests/field_menu_texture_restore_test.c -o "$OUT/$mutant/test.o"
 clang -no-pie "$OUT/$mutant/test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant/test"
 if "$OUT/$mutant/test" >"$OUT/$mutant.log" 2>&1;then echo "FAIL survived $mutant";exit 1;fi
 grep -q '^FAIL texture restoration' "$OUT/$mutant.log"
done
echo 'FIELD MENU TEXTURE RESTORE negative controls PASS count=4'
