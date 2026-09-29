#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/actor_object_state_retail_test
source=pc_port/tests/actor_object_state_retail_test.c
if [ "${1:-}" = --loop ]; then
    OUT=pc_port/build_native/actor_object_loop_retail_test
    source=pc_port/tests/actor_object_loop_retail_test.c
fi
mkdir -p "$OUT"
read -r hash _ < <(dd if=disc/field.bin bs=1 skip=$((0x80076300-0x8006faf0)) count=$((0x168)) status=none | sha256sum)
test "$hash" = e21be205a231d5806218bcd367350b0292d8cf90b1afd7c06cdb027ef228fb3f
read -r hash _ < <(dd if=disc/field.bin bs=1 skip=$((0x80075b44-0x8006faf0)) count=$((0x970)) status=none | sha256sum)
test "$hash" = a1f058817eeb40031c36bf07396c7a6dcf6bcbb620adbd1ad83f2496a1392a10
common=(-std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c "$source" -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -w -c pc_port/src/data_field.c -o "$OUT/$opt.data.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.adapter.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt.test.o" "$OUT/$opt.data.o" "$OUT/$opt.adapter.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
mutants=(skip flags scale rotation position clear table)
if [ "${1:-}" = --loop ]; then mutants=(fallback advance reset); fi
for mutant in "${mutants[@]}"; do
    range='/^static u32 FieldUpdateObjectActor(/,/^}/'
    case "$mutant" in
        skip) expression='s/return slot;/return slot + 1;/' ;;
        flags) expression='s/flags |= 1/flags = 1/' ;;
        scale) expression='s/product >> 12/product >> 16/' ;;
        rotation) expression='s/0x56) - 0xC00/0x56) + 0xC00/' ;;
        position) expression='s/node + 0x5C/node + 0x58/' ;;
        clear) expression='s/~0x200u/~0x400u/' ;;
        table) expression='s/D_801E8670\[slot\]/D_801E8670[0]/' ;;
        fallback) range='/if (actorFlags4 \& 0x2000)/,/^        }/'; expression='s/continue;/;/' ;;
        advance) range='/^void func_80075B44(/,/^}/'; expression='s/objectSlot = FieldUpdateObjectActor/(void)FieldUpdateObjectActor/' ;;
        reset) range='/^void func_80075B44(/,/^}/'; expression='s/u32 objectSlot = 0/static u32 objectSlot = 0/' ;;
    esac
    sed "$range { $expression; }" src/field/main/misc2.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w \
        "-DACTOR_RENDER_SOURCE=\"$(pwd)/$OUT/$mutant.c\"" \
        -c "$source" -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.data.o" "$OUT/O2.adapter.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "OBJECT mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'OBJECT (STATE|LOOP) FAIL' "$OUT/$mutant.log"
done
echo "OBJECT negative controls PASS: ${mutants[*]}"
