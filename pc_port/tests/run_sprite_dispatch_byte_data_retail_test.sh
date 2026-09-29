#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_dispatch_byte_data_retail_test
mkdir -p "$OUT"
for opt in O0 O2 UBSan; do
 flags=(-"$opt"); if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc -std=gnu17 -fpermissive "${flags[@]}" -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c src/slus_006.64/system/animation_scripts.c -o "$OUT/$opt.body.o"
 gcc -std=gnu17 "${flags[@]}" -Dmain=noop_main -fno-pie -Ipc_port/src -c pc_port/tests/sprite_dispatch_noop_retail_test.c -o "$OUT/$opt.guards.o"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${flags[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_byte_data_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" "$OUT/$opt.guards.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
for mutant in a2_offset bf_byte; do
 case "$mutant" in
  a2_offset) expression='s/0x3D/0x3C/' ;;
  bf_byte) expression='s/AnimationWrite16((u8\*)pSpriteData + 0x36, ((u8\*)operands)\[0\]);/((u8*)pSpriteData)[0x36] = ((u8*)operands)[0];/' ;;
 esac
 sed "$expression" src/slus_006.64/system/animation_scripts.c > "$OUT/$mutant.c"
 gcc -std=gnu17 -fpermissive -O2 -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c "$OUT/$mutant.c" -o "$OUT/$mutant.body.o"
 gcc -std=gnu17 -O2 -Dmain=noop_main -fno-pie -Ipc_port/src -c pc_port/tests/sprite_dispatch_noop_retail_test.c -o "$OUT/$mutant.guards.o"
 clang -std=c17 -O2 -Ipc_port/src -no-pie -Wl,--gc-sections pc_port/tests/sprite_dispatch_byte_data_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.body.o" "$OUT/$mutant.guards.o" -o "$OUT/$mutant.test"
 if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then echo "SPRITE BYTE mutant survived: $mutant" >&2; exit 1; fi
 rg -q 'SPRITE BYTE FAIL' "$OUT/$mutant.log"
done
echo 'SPRITE BYTE DATA negative controls PASS: a2_offset bf_byte'
echo 'SPRITE BYTE DATA O0/O2/UBSAN PASS'
