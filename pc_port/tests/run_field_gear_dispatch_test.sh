#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_gear_dispatch_test
mkdir -p "$OUT"
read -r caller_hash _ < <(dd if=disc/field.bin bs=1 \
    skip=$((0x8009fdd4 - 0x8006faf0)) count=$((0x110)) status=none | sha256sum)
test "$caller_hash" = 6af0be6042c29d442177bf2e59950fd726a6bb92769e4f59f8ff7ced6bcbd0b5
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections
        -include assert.h
        -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
        -Ipc_port/include_shim -Iinclude -Ipc_port/src
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/field_gear_dispatch_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w \
        -c src/field/main/misc6.c -o "$OUT/$opt.body.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
for mutant in actor_index fixed_slot; do
    if [ "$mutant" = actor_index ]; then
        expression='s/func_800AD4D4(result);/func_800AD4D4(D_800AFD1C);/'
    else
        expression='s/func_800ACFD0(slot);/func_800ACFD0(0);/'
    fi
    sed "$expression" src/field/main/misc6.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "FIELD GEAR DISPATCH mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'FIELD GEAR DISPATCH FAIL' "$OUT/$mutant.log"
done
echo 'FIELD GEAR DISPATCH negative controls PASS actor-index/fixed-slot'
