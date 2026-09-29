#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-direction.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
read -r actual _ < <(sha256sum disc/SLUS_006.64)
test "$actual" = dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119
sha256sum pc_port/extern/PsyCross/src/psx/LIBGTE.C \
    pc_port/extern/PsyCross/src/gte/ratan_tbl.h > "$out/native-atan-sources.sha256"
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src -DXBT_NATIVE_DIRECTION \
        -c pc_port/tests/battle_target_direction_retail_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    cc -std=gnu17 "${flags[@]}" -ffunction-sections -fdata-sections \
        -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
        -c src/battle/main35.c -o "$out/$opt.body.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$opt.test.o" "$out/$opt.cpu.o" \
        "$out/$opt.body.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
    # Compile the actual vendor body; only rename the test symbol for observation.
    g++ -std=c++17 "${flags[@]}" -fno-pie -ffunction-sections -fdata-sections \
        -DUSE_EXTENDED_PRIM_POINTERS=0 -Dratan2=PcPortDirectionNativeAtan \
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
        -fpermissive -w -include pc_port/src/port_compat.h \
        -c pc_port/extern/PsyCross/src/psx/LIBGTE.C -o "$out/$opt.atan.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -DXBT_NATIVE_DIRECTION -DXBT_REAL_NATIVE_ATAN \
        -c pc_port/tests/battle_target_direction_retail_test.c -o "$out/$opt.native.test.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$opt.native.test.o" \
        "$out/$opt.cpu.o" "$out/$opt.body.o" "$out/$opt.atan.o" -o "$out/$opt.native.test"
    "$out/$opt.native.test" 2>"$out/$opt.native.stderr" | tee "$out/$opt.native.log"
    test ! -s "$out/$opt.native.stderr"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_atan_retail_test.c -o "$out/$opt.atan.test.o"
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$opt.atan.test.o" \
        "$out/$opt.cpu.o" "$out/$opt.atan.o" -o "$out/$opt.atan.test"
    "$out/$opt.atan.test" 2>"$out/$opt.atan.stderr" | tee "$out/$opt.atan.log"
    test ! -s "$out/$opt.atan.stderr"
done
cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -DXBT_ATAN_CORRUPT_LAST \
    -c pc_port/tests/battle_target_atan_retail_test.c -o "$out/atan_bad_table.o"
clang++ -no-pie -Wl,--gc-sections "$out/atan_bad_table.o" "$out/O2.cpu.o" \
    "$out/O2.atan.o" -o "$out/atan_bad_table.test"
if "$out/atan_bad_table.test" > "$out/atan_bad_table.log" 2>&1; then
    echo 'ATAN table mutant survived' >&2; exit 1
fi
rg -q 'Assertion.*native_result.*failed' "$out/atan_bad_table.log"
echo 'ATAN last-entry native-table mutant rejected by result comparison'
cc -std=gnu17 -O2 -E -P -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
    src/battle/main35.c -o "$out/mutation-baseline.c"
for mutant in baseline actor_mask direction_mask mode2_strict unsigned_distance ties last_slot self_skip signed_product padding_write; do
    flags=(-O2)
    objects=O2
    case "$mutant" in
        baseline) expression='' ;;
        actor_mask) expression='s/arg0 \& 0xFF/arg0 \& 0x7F/' ;;
        direction_mask) expression='s/arg1 \& 0xFF/arg1/' ;;
        mode2_strict) expression='s/<= 0x200/< 0x200/' ;;
        unsigned_distance) expression='s/(s32)distance < best/distance < (u32)best/' ;;
        ties) expression='s/(s32)distance < best/(s32)distance <= best/' ;;
        last_slot) expression='s/D_800C3E90\[11\]/D_800C3E90[10]/' ;;
        self_skip) expression='s/\*entry != actor/1/' ;;
        padding_write) expression='s/return selected \& 0xFF;/D_800C3EBE[actor].pad[0] ^= 1; return selected \& 0xFF;/' ;;
        signed_product)
            expression='s/(u32)delta \* (u32)delta/delta * delta/g'
            flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
            objects=UBSan ;;
    esac
    sed "/^u32 func_80084854(/,/^}/ { $expression
}" "$out/mutation-baseline.c" > "$out/$mutant.c"
    if [ "$mutant" != baseline ] && cmp -s "$out/mutation-baseline.c" "$out/$mutant.c"; then
        echo "DIRECTION mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 "${flags[@]}" -ffunction-sections -fdata-sections \
        -c "$out/$mutant.c" -o "$out/$mutant.o"
    for boundary in hybrid native; do
        extra=()
        fixture="$out/$objects.test.o"
        if [ "$boundary" = native ]; then
            fixture="$out/$objects.native.test.o"
            extra=("$out/$objects.atan.o")
        fi
        name="$mutant.$boundary"
        clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$fixture" "$out/$objects.cpu.o" \
            "$out/$mutant.o" "${extra[@]}" -o "$out/$name.test"
        if [ "$mutant" = baseline ]; then
            "$out/$name.test" 2>"$out/$name.stderr" | tee "$out/$name.log"
            test ! -s "$out/$name.stderr"
        else
            if "$out/$name.test" > "$out/$name.log" 2>&1; then
                echo "DIRECTION mutant survived: $name" >&2; exit 1
            fi
            if [ "$mutant" = signed_product ]; then
                rg -q 'runtime error: signed integer overflow' "$out/$name.log"
            else
                rg -q 'Assertion.*(func_80084854|calls|cpu->gpr|dy>=|memcmp|arguments|result==expected_angles).*failed' "$out/$name.log"
            fi
            echo "DIRECTION mutant rejected: $name"
        fi
    done
done
