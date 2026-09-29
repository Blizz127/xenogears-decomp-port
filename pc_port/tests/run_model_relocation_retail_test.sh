#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=${MODEL_RELOCATION_OUT:-$(mktemp -d)}
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys,hashlib
sys.path.insert(0,'tools/scripts/psx')
from scan_field_anim_opcodes import DiscArchive
a=DiscArchive(Path('disc/disc1.bin'));b=a.read_entry(0x6b9);a.close()
assert hashlib.sha256(b[0xbc40:0xbce0]).hexdigest()=='5429efb01ea7bfaf525da3ccf23d5db87b17137d0d042e0cd8d5c440de3b5a87'
assert hashlib.sha256(b[0x22c:0x2d0]).hexdigest()=='ed37d38e4361912726b17fd6aeb8902f96e13650ba9caf105aa5347b7aa4e971'
assert hashlib.sha256(b[0xe18:0xec8]).hexdigest()=='e5993739ad3629bfbd3c86a337b5e52df6444b5ed0b841ac62608d62fbd3ce2d'
Path(sys.argv[1],'overlay.bin').write_bytes(b)
print('RETAIL TAIL SHA256',hashlib.sha256(b[0xbc40:0xbce0]).hexdigest())
PY
BASE=(-std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C -fno-pie -fno-builtin -ffunction-sections -fdata-sections -include assert.h -w -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in ${MODEL_RELOCATION_MODES:-O0 O2 UBSan}; do
 compiler="${CC:-gcc}"; linker="$compiler"
 flags=(-"$mode");if [ "$mode" = UBSan ];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); compiler="${UBSAN_CC:-clang}"; linker="$compiler";fi
 "$compiler" "${BASE[@]}" "${flags[@]}" -no-pie -Wl,--gc-sections pc_port/tests/model_relocation_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
 "$OUT/$mode" "$OUT/overlay.bin" | tee "$OUT/$mode.log"
done
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
s=Path('pc_port/src/field_object_overlay.c').read_text();out=Path(sys.argv[1])
a=s.index('static void OvlyFinalizeModelCopy(');b=s.index('void func_801E742C(',a);body=s[a:b]
controls={
 'unrelocate':('(void)func_8002C4BC(copy);','/* omitted */'),
 'allocation-end':('HeapAlloc(size, 0)','HeapAlloc(size, 1)'),
 'free-copy':('HeapFree(copy);','/* omitted */'),
 'free-table':('HeapFree(tab->ptrs);','/* omitted */'),
 'copy-length':('memcpy(relocated, copy, size);','memcpy(relocated, copy, size - 4);'),
 'flag-bit':('flags & 2','flags & 1'),
 'owner':('(u32)(uintptr_t)relocated;','(u32)(uintptr_t)copy;'),
}
for name,(old,new) in controls.items():
 assert body.count(old)==1,(name,body.count(old))
 (out/(name+'.c')).write_text(s[:a]+body.replace(old,new)+s[b:])
PY
for control in unrelocate allocation-end free-copy free-table copy-length flag-bit owner; do
 gcc "${BASE[@]}" -O2 "-DMODEL_RELOCATION_SOURCE=\"$OUT/$control.c\"" -no-pie -Wl,--gc-sections pc_port/tests/model_relocation_retail_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$control"
 rc=0; "$OUT/$control" "$OUT/overlay.bin" > "$OUT/$control.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! grep -q '^MODEL RELOCATION FAIL ' "$OUT/$control.log";then cat "$OUT/$control.log";echo "MODEL RELOCATION undetected control $control";exit 1;fi
 echo "MODEL RELOCATION CONTROL REJECTED $control"
done
