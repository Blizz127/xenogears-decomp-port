#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=${MODEL_UNRELOCATE_OUT:-$(mktemp -d)}
SOURCE=${MODEL_UNRELOCATE_SOURCE:-src/slus_006.64/system/temp2.c}
mkdir -p "$OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
p=Path('disc/SLUS_006.64').read_bytes()
assert sha256(p).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
print('MODEL UNRELOCATE RETAIL SHA256',sha256(p[0x1ccbc:0x1cd9c]).hexdigest())
PY
COMMON=(-include assert.h -std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in ${MODEL_UNRELOCATE_MODES:-O0 O2 UBSan}; do
 compiler="${CC:-gcc}"; linker="$compiler"
 flags=(-"$mode"); if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); linker="${UBSAN_CC:-clang}"; fi
 "$compiler" "${COMMON[@]}" "${flags[@]}" -w -c "$SOURCE" -o "$OUT/$mode.source.o"
 "$compiler" "${COMMON[@]}" "${flags[@]}" -c pc_port/tests/model_unrelocate_retail_test.c -o "$OUT/$mode.test.o"
 "$compiler" "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 "$linker" -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
 "$OUT/$mode" disc/SLUS_006.64 | tee "$OUT/$mode.log"
done
if [ -f "$OUT/O2.test.o" ]; then
 python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text();out=Path(sys.argv[2])
a=s.index('s32 func_8002C4BC(');b=s.index('void func_8002C59C(',a);body=s[a:b]
controls={
 'fourth-pointer':('*(u32*)(pEntry - 0x08) -= (u32)pBlock;','/* omitted */'),
 'nested-pointer':('*(u32*)pEntry -= (u32)pBlock;','/* omitted */'),
 'nested-entry':('*(u32*)(pRecord + 8) -= (u32)pBlock;','/* omitted */'),
 'flag-clear':('flags & ~1','flags'),
 'return-count':('return count;','return count + 1;'),
}
for name,(old,new) in controls.items():
 assert old in body,name
 (out/(name+'.c')).write_text(s[:a]+body.replace(old,new)+s[b:])
PY
 for control in fourth-pointer nested-pointer nested-entry flag-clear return-count; do
  gcc "${COMMON[@]}" -O2 -w -c "$OUT/$control.c" -o "$OUT/$control.source.o"
  gcc -no-pie -O2 -Wl,--gc-sections "$OUT/$control.source.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/$control"
  rc=0; "$OUT/$control" disc/SLUS_006.64 > "$OUT/$control.log" 2>&1 || rc=$?
  if [ "$rc" != 1 ] || ! grep -q '^MODEL UNRELOCATE FAIL ' "$OUT/$control.log"; then
   cat "$OUT/$control.log"; echo "MODEL UNRELOCATE undetected control $control"; exit 1
  fi
  echo "MODEL UNRELOCATE CONTROL REJECTED $control"
 done
fi
