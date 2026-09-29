#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mktemp -d pc_port/build_native/gfx_deferred_free.XXXXXXXX)
echo "GFX deferred-free artifacts: $out"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
import re
b=Path('disc/SLUS_006.64').read_bytes()
a=Path('asm/slus_006.64/matchings/system/temp1e/func_800250E0.s').read_text()
rows=re.findall(r'/\* ([0-9A-F]+) ([0-9A-F]+) ([0-9A-F]+) \*/',a)
assert len(rows)==40
for _,address,word in rows:
    offset=int(address,16)-0x8000f800
    assert b[offset:offset+4]==bytes.fromhex(word)
assert sha256(b[0x158e0:0x15980]).hexdigest()=='305e3a5c2f2ea2284536e0f47b7760caaa1827edd657d98faf06ab2c27834ac3'
print('GFX 800250E0: all 160 annotated instruction bytes match disc')
PY
base=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
      -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [[ "$opt" == UBSan ]]; then flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${base[@]}" "${flags[@]}" -w -fpermissive -include assert.h \
        -c src/slus_006.64/system/temp1e.c -o "$out/$opt.body.o"
    gcc "${base[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        pc_port/tests/gfx_deferred_free_retail_test.c pc_port/src/battle_mips_adapter.c \
        "$out/$opt.body.o" -no-pie -Wl,--gc-sections -o "$out/$opt"
    "$out/$opt"
done
python3 - "$out" <<'PY'
from pathlib import Path
import sys
source=Path('src/slus_006.64/system/temp1e.c').read_text()
edits={
    'head': ('pCurEntry = SpriteRenderAddress((uintptr_t)pCurEntry);',
             'pCurEntry = pCurEntry;'),
    'payload': ('HeapFree(SpriteRenderAddress(*(u32*)(pCurEntry + 0x0)));',
                'HeapFree((void*)(uintptr_t)*(u32*)(pCurEntry + 0x0));'),
    'next': ('pCurEntry = SpriteRenderAddress(*(u32*)(pCurEntry + 0x4));',
             'pCurEntry = (u8*)(uintptr_t)*(u32*)(pCurEntry + 0x4);'),
}
for name,(old,new) in edits.items():
    assert source.count(old)==1
    Path(sys.argv[1],name+'.c').write_text(source.replace(old,new))
PY
for mutant in head payload next; do
    gcc "${base[@]}" -O0 -w -fpermissive -include assert.h \
        -c "$out/$mutant.c" -o "$out/$mutant.body.o"
    gcc "${base[@]}" -O0 pc_port/tests/gfx_deferred_free_retail_test.c \
        pc_port/src/battle_mips_adapter.c "$out/$mutant.body.o" \
        -no-pie -Wl,--gc-sections -o "$out/$mutant"
    rc=0
    "$out/$mutant" > "$out/$mutant.log" 2>&1 || rc=$?
    if [[ "$mutant" == payload ]]; then
        test "$rc" -eq 134
        grep -q 'memcmp(calls,expected_calls' "$out/$mutant.log"
    else
        test "$rc" -eq 139
    fi
    echo "GFX deferred-free negative control detected: $mutant"
done
