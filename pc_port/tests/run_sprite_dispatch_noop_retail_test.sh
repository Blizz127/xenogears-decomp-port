#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_dispatch_noop_retail_test
mkdir -p "$OUT"
python3 - <<'PY'
from pathlib import Path
from hashlib import sha256
b=Path('disc/SLUS_006.64').read_bytes()
for a,z,h in [(0x800183d8,0x800185a4,'ee73be97683cbf2affa5db0d128e5083ac4b273da3bc5c2f0bfc12bfb75ee0f8'),(0x8001fbe4,0x80021ad8,'7431f354f172d1b450e6c231528af93546595e8aa6d750d10ca489d75e88442c')]:
 assert sha256(b[a-0x8000f800:z-0x8000f800]).hexdigest()==h
PY
for opt in O0 O2 UBSan; do
 flags=(-"$opt")
 if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 common=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections "${flags[@]}")
 # Match the existing port's permissive compilation of this legacy TU;
 # warnings remain visible. The new oracle test and CPU compile with Werror.
 gcc -std=gnu17 -fpermissive "${common[@]}" -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c src/slus_006.64/system/animation_scripts.c -o "$OUT/$opt.body.o"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${common[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_noop_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in missing invented width route; do
 case "$mutant" in
  missing) expression='s/case 0x8B: case 0x8E:/case 0x8E:/' ;;
  invented) expression='s/case 0x8B: case 0x8E:/case 0x8A: case 0x8B: case 0x8E:/' ;;
  width) expression='s/switch ((u8)opcodeIndex)/switch (opcodeIndex)/' ;;
  route) expression='s/if (AnimationScriptOpcodeIsNoop(opcodeIndex))/if (0)/' ;;
 esac
 sed "$expression" src/slus_006.64/system/animation_scripts.c > "$OUT/$mutant.c"
 gcc -std=gnu17 -fpermissive -O2 -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
 clang -std=c17 -O2 -fno-pie -no-pie -Ipc_port/src -Wl,--gc-sections pc_port/tests/sprite_dispatch_noop_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
  echo "SPRITE NOOP mutant survived: $mutant" >&2; exit 1
 fi
 if [ "$mutant" = route ]; then
  rg -q 'func_8001FBE4 dispatch path is not implemented' "$OUT/$mutant.log"
 else
  rg -q 'SPRITE NOOP FAIL' "$OUT/$mutant.log"
 fi
done
echo 'SPRITE NOOP negative controls PASS: missing invented width route'
