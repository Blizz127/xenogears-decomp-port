#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-ranges.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan PIE; do
    flags=(-"$opt")
    link_flags=(-no-pie)
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    if [ "$opt" = PIE ]; then
        flags=(-O2 -fPIE -DXBT_REQUIRE_HIGH_LIST)
        link_flags=(-pie)
    fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src -DXBT_NATIVE_ACTOR_RANGE \
        -c pc_port/tests/battle_target_ranges_retail_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    for unit in main35 main41; do
        cc -std=gnu17 "${flags[@]}" -ffunction-sections -fdata-sections \
            -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
            -c "src/battle/$unit.c" -o "$out/$opt.$unit.o"
    done
    clang "${link_flags[@]}" "${flags[@]}" -Wl,--gc-sections "$out/$opt.test.o" "$out/$opt.cpu.o" \
        "$out/$opt.main35.o" "$out/$opt.main41.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
cc -std=gnu17 -O2 -E -P -DXENO_PC_PORT -DSKIP_ASM -Iinclude -Ipc_port/include_shim \
    src/battle/main35.c -o "$out/mutation-baseline.c"
cc -std=gnu17 -O2 -fPIE -ffunction-sections -fdata-sections \
    -c "$out/mutation-baseline.c" -o "$out/PIE.baseline.o"
clang -pie -Wl,--gc-sections "$out/PIE.test.o" "$out/PIE.cpu.o" \
    "$out/PIE.baseline.o" "$out/PIE.main41.o" -o "$out/PIE.baseline.test"
"$out/PIE.baseline.test" 2>"$out/PIE.baseline.stderr" | tee "$out/PIE.baseline.log"
test ! -s "$out/PIE.baseline.stderr"
sed '/^u32 func_80084750(/,/^}/ s/uintptr_t clear =/u32 clear =/' \
    "$out/mutation-baseline.c" > "$out/cursor_narrow.c"
if cmp -s "$out/mutation-baseline.c" "$out/cursor_narrow.c"; then
    echo 'CURSOR WIDTH mutation made no change' >&2; exit 1
fi
cc -std=gnu17 -O2 -fPIE -ffunction-sections -fdata-sections \
    -c "$out/cursor_narrow.c" -o "$out/cursor_narrow.o"
objdump -dr --disassemble=func_80084750 "$out/cursor_narrow.o" > "$out/cursor_narrow.dis"
rg -q '^ +20:\s+67 c6 00 ff\s+' "$out/cursor_narrow.dis"
cc -std=gnu17 -O2 -fPIE -Wall -Wextra -Werror -Ipc_port/src \
    -DXBT_NATIVE_ACTOR_RANGE -DXBT_REQUIRE_HIGH_LIST -DXBT_EXPECT_CURSOR_FAULT \
    -c pc_port/tests/battle_target_ranges_retail_test.c -o "$out/cursor_narrow.test.o"
clang -pie -Wl,--gc-sections "$out/cursor_narrow.test.o" "$out/PIE.cpu.o" \
    "$out/cursor_narrow.o" "$out/PIE.main41.o" -o "$out/cursor_narrow.test"
if "$out/cursor_narrow.test" > "$out/cursor_narrow.log" 2>&1; then
    echo 'CURSOR WIDTH mutant survived' >&2; exit 1
else
    result=$?
fi
test "$result" = 79
rg -q '^CURSOR WIDTH expected truncated-address store rejected$' "$out/cursor_narrow.log"
echo 'CURSOR WIDTH mutant rejected at expected low address'
for mutant in baseline actor_zero range_zero eligibility_zero mask_replace clear_short; do
    case "$mutant" in
        baseline) expression='' ;;
        actor_zero) expression='s/arg0 \& 0xFF/0/' ;;
        range_zero) expression='s/i = 3/i = 0/' ;;
        eligibility_zero) expression='s/eligible \& 0xFF/eligible \& 0/' ;;
        mask_replace) expression='s/D_800C3D64 |= mask/D_800C3D64 = mask/' ;;
        clear_short) expression='s/i = 11/i = 10/' ;;
    esac
    sed "/^u32 func_80084750(/,/^}/ { $expression
}" "$out/mutation-baseline.c" > "$out/$mutant.c"
    if [ "$mutant" != baseline ] && cmp -s "$out/mutation-baseline.c" "$out/$mutant.c"; then
        echo "ACTOR RANGE mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -ffunction-sections -fdata-sections -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$out/O2.test.o" "$out/O2.cpu.o" \
        "$out/$mutant.o" "$out/O2.main41.o" -o "$out/$mutant.test"
    if [ "$mutant" = baseline ]; then
        "$out/$mutant.test" 2>"$out/$mutant.stderr" | tee "$out/$mutant.log"
        test ! -s "$out/$mutant.stderr"
    else
        if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
            echo "ACTOR RANGE mutant survived: $mutant" >&2; exit 1
        fi
        rg -q 'Assertion.*(native_result|D_800D3274|memcmp\(D_800C3E90).*failed' "$out/$mutant.log"
        echo "ACTOR RANGE mutant rejected: $mutant"
    fi
done
