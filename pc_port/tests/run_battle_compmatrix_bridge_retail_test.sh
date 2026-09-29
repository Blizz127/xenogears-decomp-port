#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d pc_port/build_native/battle_compmatrix.XXXXXXXX)
export OUT
python3 - <<'PIN'
from pathlib import Path
import hashlib,json,os
b=Path('disc/SLUS_006.64').read_bytes()
assert hashlib.sha256(b).hexdigest()=='dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119'
assert hashlib.sha256(b[0x39b1c:0x39c7c]).hexdigest()=='484fff1d2984fac9730786048ad50065c006672118c2d6b4698dfb787d7b155c'
pins={'retail_address':'0x8004931c','retail_length':352,'retail_sha256':hashlib.sha256(b[0x39b1c:0x39c7c]).hexdigest()}
for f in ['pc_port/src/battle_mips_runtime.c','pc_port/src/battle_mips_adapter.c','pc_port/extern/PsyCross/src/psx/LIBGTE.C','pc_port/tests/battle_compmatrix_bridge_retail_test.c','pc_port/tests/run_battle_compmatrix_bridge_retail_test.sh']:
 pins[f]=hashlib.sha256(Path(f).read_bytes()).hexdigest()
Path(os.environ['OUT'],'pins.json').write_text(json.dumps(pins,indent=2)+'\n')
PIN
common=(-fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
 flags=(-"$mode")
 if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 for src in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
  g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$src" -o "$OUT/$mode.${src##*/}.o"
 done
 gcc -std=gnu17 "${common[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/build_native -Wall -Wextra -Werror -c pc_port/tests/battle_compmatrix_bridge_retail_test.c -o "$OUT/$mode.test.o"
 gcc -std=gnu17 "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode.cpu.o"
 clang++ -no-pie "${flags[@]}" -Wl,--gc-sections -Wl,--export-dynamic-symbol=CompMatrix "$OUT/$mode.test.o" "$OUT/$mode.cpu.o" "$OUT/$mode.LIBGTE.C.o" "$OUT/$mode.INLINE_C.C.o" "$OUT/$mode.PsyX_GTE.cpp.o" -ldl -o "$OUT/$mode.test"
 "$OUT/$mode.test" > "$OUT/$mode.log" 2>&1
 cat "$OUT/$mode.log"
done
printf 'COMPMATRIX artifacts: %s\n' "$OUT"

python3 - <<'MUTATE'
from pathlib import Path
import os
out=Path(os.environ['OUT']);src=Path('pc_port/src/battle_mips_runtime.c').read_text()
a=src.index('    if (resolved->address == 0x8004931cu) {')
b=src.index('    for (i = 0; i < 4; i++) {',a)
block=src[a:b]
for name,replacement in [('no-typed-bridge',''),('no-low-alias',block.replace('address |= 0x80000000u;','address = address;')),('wrong-return',block.replace('cpu->gpr[2] = cpu->gpr[6];','cpu->gpr[2] = (uint32_t)(uintptr_t)matrices[2];'))]:
 assert replacement!=block
 (out/(name+'.c')).write_text(src[:a]+replacement+src[b:])
lib=Path('pc_port/extern/PsyCross/src/psx/LIBGTE.C').read_bytes()
line=b'\t((unsigned char*)m2)[0x12] = ((unsigned char*)m2)[0x13] = (unsigned char)matrixTail;'
assert lib.count(line)==1
(out/'missing-word-store.C').write_bytes(lib.replace(line,b'\t(void)matrixTail;'))
MUTATE
ulimit -c 0
for mutant in no-typed-bridge no-low-alias wrong-return missing-word-store; do
 runtime_flags=()
 library="$OUT/O2.LIBGTE.C.o"
 if [ "$mutant" = missing-word-store ]; then
  g++ -std=c++17 "${common[@]}" -O2 -fpermissive -w -include pc_port/src/port_compat.h -Ipc_port/extern/PsyCross/src/psx -c "$OUT/missing-word-store.C" -o "$OUT/missing-word-store.o"
  library="$OUT/missing-word-store.o"
 else
  runtime_flags+=("-DBATTLE_RUNTIME_SOURCE=\"$(pwd)/$OUT/$mutant.c\"")
 fi
 gcc -std=gnu17 "${common[@]}" -O2 "${runtime_flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h -Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/build_native -c pc_port/tests/battle_compmatrix_bridge_retail_test.c -o "$OUT/$mutant.test.o"
 clang++ -no-pie -Wl,--gc-sections -Wl,--export-dynamic-symbol=CompMatrix "$OUT/$mutant.test.o" "$OUT/O2.cpu.o" "$library" "$OUT/O2.INLINE_C.C.o" "$OUT/O2.PsyX_GTE.cpp.o" -ldl -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "COMPMATRIX FAIL control survived: $mutant" >&2
  exit 1
 fi
 echo "COMPMATRIX control rejected: $mutant"
done
