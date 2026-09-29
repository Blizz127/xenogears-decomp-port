#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_gear_board_retail_test
entry=800AD4D4
start=0x800ad4d4
length=0x3c4
expected_hash=98fa2e22aeb3ccae9c04d4661febd4dd771e108639b87b8f5a2db1e205bf5f0f
defines=()
if [ "${1:-}" = --exit ]; then
    OUT=pc_port/build_native/field_gear_exit_retail_test
    entry=800ACFD0
    start=0x800acfd0
    length=0x504
    expected_hash=4a0c2c56dd0051c0c7db399d66b66c82e304869bd3b20058dfa349b96c953ac5
    defines=(-DTEST_GEAR_EXIT)
fi
mkdir -p "$OUT"
read -r routine_hash _ < <(dd if=disc/field.bin bs=1 \
    skip=$((start - 0x8006faf0)) count=$((length)) status=none | sha256sum)
test "$routine_hash" = "$expected_hash"
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    common=("${defines[@]}" -std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
        -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
        -Ipc_port/include_shim -Iinclude -Ipc_port/src
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/field_gear_board_retail_test.c -o "$OUT/$opt.test.o"
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/battle_mips_adapter.c -o "$OUT/$opt.cpu.o"
    gcc "${common[@]}" "${flags[@]}" -fpermissive -w \
        -c src/field/scripts/virtual_machine.c -o "$OUT/$opt.body.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
mutants=(position flags particle_mode)
if [ "$entry" = 800ACFD0 ]; then mutants+=(animation copied_angle); fi
for mutant in "${mutants[@]}"; do
    case "$mutant" in
        position) expression='s/destination\[2\] = \*(u32\*)(data + 0x28);/destination[2] = 0;/' ;;
        flags) expression='s/| 0x200u/| 0x400u/' ;;
        particle_mode) expression='s/FieldParticleActorStop(D_8006F990\[partySlot\], 0);/FieldParticleActorStop(D_8006F990[partySlot], 1);/' ;;
        animation) expression='s/party->pSpriteData, 6, party/party->pSpriteData, 0, party/' ;;
        copied_angle) expression='s/(partyData + 0x108) = \*(u16\*)(data + 0x108)/(partyData + 0x108) = *(u16*)(data + 0x106)/' ;;
    esac
    if [ "$entry" = 800ACFD0 ]; then
        case "$mutant" in
            flags) expression='s/| 0x400u/| 0x200u/' ;;
        esac
    fi
    sed "/^void func_$entry(/,/^}/ { $expression; }" \
        src/field/scripts/virtual_machine.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" \
        "$OUT/O2.cpu.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "GEAR BOARD RETAIL mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'GEAR BOARD RETAIL FAIL' "$OUT/$mutant.log"
done
echo "GEAR RETAIL $entry negative controls PASS ${mutants[*]}"
