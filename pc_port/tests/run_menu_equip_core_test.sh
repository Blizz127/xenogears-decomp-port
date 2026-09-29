#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/menu_equip_core
mkdir -p "$OUT"
python3 - <<'PYGEN'
from pathlib import Path
import hashlib,re
b=Path('disc/menu.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d'
a=next(Path('asm/menu').glob('*matchings/main/misc*/func_801E05D0.s')).read_text()
raw=b''.join(bytes.fromhex(w) for addr,w in re.findall(r'/\* [0-9A-F]+ ([0-9A-F]+) ([0-9A-F]{8}) \*/',a) if 0x801e05d0<=int(addr,16)<0x801e0f78)
assert raw==b[0x1b5d0:0x1bf78]
s=Path('src/menu/main/misc.c').read_text();a=s.index('void func_801E05D0(u8')
# Brace-aware extraction: the body contains inner #ifdef blocks, so a naive
# '#endif' search truncates it. Scan from the opening brace instead.
i=s.index('{',a);depth=0;z=None;n=len(s)
in_str=in_chr=in_lc=in_bc=False;esc=False
while i<n:
    ch=s[i];nx=s[i+1] if i+1<n else ''
    if in_lc:
        if ch=='\n':in_lc=False
    elif in_bc:
        if ch=='*' and nx=='/':in_bc=False;i+=1
    elif in_str:
        if esc:esc=False
        elif ch=='\\':esc=True
        elif ch=='"':in_str=False
    elif in_chr:
        if esc:esc=False
        elif ch=='\\':esc=True
        elif ch=="'":in_chr=False
    else:
        if ch=='/' and nx=='/':in_lc=True;i+=1
        elif ch=='/' and nx=='*':in_bc=True;i+=1
        elif ch=='"':in_str=True
        elif ch=="'":in_chr=True
        elif ch=='{':depth+=1
        elif ch=='}':
            depth-=1
            if depth==0:z=i+1;break
    i+=1
assert z is not None
Path('pc_port/build_native/menu_equip_core/equip_core_body.inc').write_text(s[a:z]+'\n')
PYGEN
BASE=(-std=gnu17 -fno-pie -include assert.h -DXENO_PC_PORT -DUSE_EXTENDED_PRIM_POINTERS=0 -D_LANGUAGE_C -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for mode in O0 O2 UBSan;do
 flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all);fi
 gcc "${BASE[@]}" "${flags[@]}" -I"$OUT" -c pc_port/tests/menu_equip_core_test.c -o "$OUT/$mode.test.o"
 gcc "${BASE[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.mips.o"
 clang -no-pie "${flags[@]}" "$OUT/$mode.test.o" "$OUT/$mode.mips.o" -o "$OUT/$mode"
 "$OUT/$mode" | tee "$OUT/$mode.log"
done

python3 - <<'PYGEN'
from pathlib import Path
p=Path('pc_port/build_native/menu_equip_core');s=(p/'equip_core_body.inc').read_text()
mutants={'cursor_x':('p->x0=0x8C;','p->x0=0x8D;'),'scroll_clamp':('if(page>count)page=count;','if(page>count)page=0;'),'group_gate':('currentCharacterIDs[selected]==4','currentCharacterIDs[selected]==0'),'navigator_direction':('u8 direction=g_Menu->input==10;','u8 direction=g_Menu->input==9;'),'skip_preview':('if(preview)func_801DFB68','if(0)func_801DFB68'),'exit_cleanup':('func_801D2484();func_801DB340(0);','func_801D2484();')}
for n,(old,new) in mutants.items():
 assert s.count(old)==1
 (p/n).mkdir(exist_ok=True);(p/n/'equip_core_body.inc').write_text(s.replace(old,new))
PYGEN
for mutant in cursor_x scroll_clamp group_gate navigator_direction skip_preview exit_cleanup;do
 gcc "${BASE[@]}" -O2 -I"$OUT/$mutant" -c pc_port/tests/menu_equip_core_test.c -o "$OUT/$mutant/test.o"
 clang -no-pie "$OUT/$mutant/test.o" "$OUT/O2.mips.o" -o "$OUT/$mutant/test"
 if "$OUT/$mutant/test" >"$OUT/$mutant.log" 2>&1;then echo "FAIL survived $mutant";exit 1;fi
 grep -q '^FAIL differential' "$OUT/$mutant.log"
done
echo 'EQUIP CORE negative controls PASS count=6'
