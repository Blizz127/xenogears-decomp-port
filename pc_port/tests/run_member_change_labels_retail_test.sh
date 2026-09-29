#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
SOURCE=${MEMBER_LABELS_SOURCE:-"$PWD/src/member_change_menu/main/misc.c"}
OUT=${MEMBER_LABELS_OUT:-$(mktemp -d /tmp/xeno-member-labels-test.XXXXXXXX)}
mkdir -p "$OUT"
printf 'MEMBER LABELS source=%s artifacts=%s\n' "$SOURCE" "$OUT"
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
source=Path(sys.argv[1]).resolve(); out=Path(sys.argv[2])
p=Path('disc/member_change_menu.bin'); b=p.read_bytes()
assert sha256(b).hexdigest()=='3b9e2b890c27ae0de97fe343ac75cd35c05fe7f9605ae387d2166da0be78109c'
paths=[source,p,Path('pc_port/tests/member_change_labels_retail_test.c'),Path('pc_port/tests/run_member_change_labels_retail_test.sh'),Path('pc_port/src/battle_mips_adapter.c'),Path('include/system/menu.h')]
(out/'provenance.json').write_text(json.dumps({'scope':'installed label drawer vs432 retail bytes; adversarial helper pointer rebinding and native initializer layout','source_pins':{str(p.resolve()):sha256(p.read_bytes()).hexdigest() for p in paths}},indent=2)+'\n')
PY
COMMON=(-std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in ${MEMBER_LABELS_MODES:-O0 O2 UBSan}; do
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c "$SOURCE" -o "$OUT/$mode.source.o"
    objcopy --weaken-symbol=func_801C57A0 --weaken-symbol=func_801C5724 "$OUT/$mode.source.o"
    gcc "${COMMON[@]}" "${flags[@]}" -fpermissive -w -c pc_port/tests/member_change_labels_retail_test.c -o "$OUT/$mode.test.o"
    clang "${COMMON[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
    "$OUT/$mode" disc/member_change_menu.bin >"$OUT/$mode.log" 2>&1 || { cat "$OUT/$mode.log"; exit 1; }
    cat "$OUT/$mode.log"
done
if [ "${MEMBER_LABELS_SKIP_CONTROLS:-0}" != 1 ]; then
    python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
s=Path(sys.argv[1]).read_text();out=Path(sys.argv[2])
a=s.index('void func_801C59E0(');b=s.index('extern u8 D_801CB400[];',a)
body=s[a:b]; work='g_Menu->unk4E0[0].pVramBuffer'
assert body.count(work)==3
cached=body.replace(work,'cachedWork').replace('    s32 i;',f'    void* cachedWork = {work};\n    s32 i;',1)
mutants={'cached-work':s[:a]+cached+s[b:]}
table='g_Menu->unk2E0'
assert body.count(table)==2
cached=body.replace(table,'cachedTable').replace('    s32 i;',f'    void* cachedTable = {table};\n    s32 i;',1)
mutants['cached-string-table']=s[:a]+cached+s[b:]
for name,old,new in (
    ('wrong-height','item->vramDest.h = 13;','item->vramDest.h = 12;'),
    ('missing-rectangle-copy','base[i + 1].vramDest = item->vramDest;','(void)item;'),
):
    assert body.count(old)==1,(name,body.count(old))
    mutants[name]=s[:a]+body.replace(old,new,1)+s[b:]
old=work+' = HeapAlloc(0x38E, 0);'
assert s.count(old)==1
mutants['wrong-initializer-storage']=s.replace(old,'*(void**)((u8*)g_Menu + 0x558) = HeapAlloc(0x38E, 0);',1)
for name,text in mutants.items(): (out/(name+'.c')).write_text(text)
(out/'controls.json').write_text(json.dumps({name:sha256(text.encode()).hexdigest() for name,text in mutants.items()},indent=2)+'\n')
PY
    for control in cached-work cached-string-table wrong-height missing-rectangle-copy wrong-initializer-storage; do
        gcc "${COMMON[@]}" -O2 -fpermissive -w -c "$OUT/$control.c" -o "$OUT/$control.source.o"
        objcopy --weaken-symbol=func_801C57A0 --weaken-symbol=func_801C5724 "$OUT/$control.source.o"
        clang -no-pie -O2 -Wl,--gc-sections "$OUT/$control.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/$control"
        rc=0
        "$OUT/$control" disc/member_change_menu.bin >"$OUT/$control.log" 2>&1 || rc=$?
        if [ "$rc" != 1 ] || ! rg -q '^MEMBER LABELS FAIL ' "$OUT/$control.log"; then
            cat "$OUT/$control.log"
            printf 'MEMBER LABELS control did not fail semantically: %s rc=%s\n' "$control" "$rc" >&2
            exit 1
        fi
        printf 'MEMBER LABELS control rejected: %s\n' "$control"
    done
fi
python3 - "$OUT" <<'PY'
from pathlib import Path
from hashlib import sha256
import json,sys
p=json.loads((Path(sys.argv[1])/'provenance.json').read_text())
for path,expected in p['source_pins'].items(): assert sha256(Path(path).read_bytes()).hexdigest()==expected,path
print('MEMBER LABELS source pins unchanged')
PY
