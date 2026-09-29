#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${BIND_ARGS_OUT:-$(mktemp -d)}
SOURCE=${BIND_ARGS_SOURCE:-src/field/main/misc2.c}
mkdir -p "$OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
b=Path('disc/field.bin').read_bytes()
assert sha256(b).hexdigest()=='38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc'
PY
cat > "$OUT/prototypes.h" <<'HEADER'
#include "common.h"
#include "psyq/libgte.h"
void FieldMatrixCopy(MATRIX*,MATRIX*);
void FieldMatrixCopyTranslation(MATRIX*,MATRIX*);
void FieldMatrixCopyTransform(MATRIX*,MATRIX*);
void func_80281B00(void*);
HEADER
COMMON=(-include "$OUT/prototypes.h" -include assert.h -std=gnu17 -fno-pie -fno-builtin -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode");if [ "$mode" = UBSan ];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
 clang "${COMMON[@]}" "${flags[@]}" -fno-inline-functions -Wno-everything -c "$SOURCE" -o "$OUT/$mode.source.o"
 objcopy --weaken-symbol=func_80076A74 "$OUT/$mode.source.o"
 clang "${COMMON[@]}" "${flags[@]}" -Wno-unused-parameter -c pc_port/tests/field_sprite_bind_args_retail_test.c -o "$OUT/$mode.test.o"
 clang "${COMMON[@]}" "${flags[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$mode.source.o" "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" -o "$OUT/$mode"
 "$OUT/$mode" disc/field.bin
done
python3 - "$SOURCE" "$OUT" <<'PY'
from pathlib import Path
import sys,json,hashlib
p=Path(sys.argv[1]);out=Path(sys.argv[2]);s=p.read_text()
start=s.index('void func_80076AC0(');end=s.index('\n}',start)+2;body=s[start:end]
for name,a,b in [('selector','+ 0x126) = skinSelector;','+ 0x126) = texPageOffset;'),('page-flags','| (texPageOffset & 0xF);','| (skinSelector & 0xF);'),('allocator','if (texPageOffset == 0)', 'if (skinSelector == 0)')]:
 assert body.count(a)==1,name
 (out/(name+'.c')).write_text(s[:start]+body.replace(a,b)+s[end:])
paths=[p,Path('disc/field.bin'),Path('pc_port/tests/field_sprite_bind_args_retail_test.c'),Path('pc_port/tests/run_field_sprite_bind_args_retail_test.sh'),Path('pc_port/src/battle_mips_adapter.c')]
(out/'pins.json').write_text(json.dumps({'sha256':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},'scope':'real retail instructions from80076AC0 to first allocator call; allocator boundaries stop execution; later binder setup untested; full native TU with test-only forward declarations and weakened unreachable callback'},indent=2)+'\n')
PY
for control in selector page-flags allocator;do
 clang "${COMMON[@]}" -O2 -fno-inline-functions -Wno-everything -c "$OUT/$control.c" -o "$OUT/$control.o"
 objcopy --weaken-symbol=func_80076A74 "$OUT/$control.o"
 clang -no-pie -O2 -Wl,--gc-sections "$OUT/$control.o" "$OUT/O2.test.o" "$OUT/O2.cpu.o" -o "$OUT/$control"
 rc=0
 "$OUT/$control" disc/field.bin > "$OUT/$control.log" 2>&1 || rc=$?
 if [ "$rc" != 1 ] || ! rg -q '^BIND ARGS FAIL' "$OUT/$control.log";then cat "$OUT/$control.log";exit 1;fi
 echo "BIND ARGS CONTROL REJECTED $control"
done
