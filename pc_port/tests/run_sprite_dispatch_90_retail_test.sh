#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/sprite_dispatch_90_retail_test
mkdir -p "$OUT"
for opt in O0 O2 UBSan; do
 flags=(-"$opt"); if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
 gcc -std=gnu17 -fpermissive "${flags[@]}" -include stdint.h -D_LANGUAGE_C -fno-pie -ffunction-sections -fdata-sections -DXENO_PC_PORT -DSKIP_ASM -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -c src/slus_006.64/system/animation_scripts.c -o "$OUT/$opt.body.o"
 gcc -std=gnu17 "${flags[@]}" -Dmain=noop_main -fno-pie -Ipc_port/src -c pc_port/tests/sprite_dispatch_noop_retail_test.c -o "$OUT/$opt.guards.o"
 clang -std=c17 -Wall -Wextra -Werror -Ipc_port/src -no-pie "${flags[@]}" -Wl,--gc-sections pc_port/tests/sprite_dispatch_90_retail_test.c pc_port/src/battle_mips_adapter.c "$OUT/$opt.body.o" "$OUT/$opt.guards.o" -o "$OUT/$opt.test"
 "$OUT/$opt.test"
done
echo 'SPRITE 90 O0/O2/UBSAN PASS'
