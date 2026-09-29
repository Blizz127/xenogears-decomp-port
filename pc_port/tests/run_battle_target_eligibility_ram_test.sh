#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-eligibility-ram.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -DXBT_TEST_RAM_TRACE -c pc_port/src/battle_target_eligibility_ram.c -o "$out/$opt.body.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    for suite in retail bounds; do
        source=pc_port/tests/battle_target_eligibility_retail_test.c
        if [ "$suite" = bounds ]; then source=pc_port/tests/battle_target_eligibility_ram_bounds_test.c; fi
        cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
            -DXBT_TEST_PACKED_RAM -c "$source" -o "$out/$opt.$suite.o"
        clang -no-pie "${flags[@]}" "$out/$opt.body.o" "$out/$opt.cpu.o" \
            "$out/$opt.$suite.o" -o "$out/$opt.$suite.test"
        "$out/$opt.$suite.test" 2>"$out/$opt.$suite.stderr" | tee "$out/$opt.$suite.log"
        test ! -s "$out/$opt.$suite.stderr"
    done
    # Normal builds must not retain the test observer dependency.
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_target_eligibility_ram.c -o "$out/$opt.plain.o"
    nm "$out/$opt.plain.o" > "$out/$opt.plain.symbols"
    if rg -q PcPortBattleTargetRamObserve "$out/$opt.plain.symbols"; then
        echo "ELIGIBILITY RAM observer leaked into normal build" >&2; exit 1
    fi
    clang -no-pie "${flags[@]}" "$out/$opt.plain.o" "$out/$opt.bounds.o" \
        -o "$out/$opt.plain.test"
    "$out/$opt.plain.test" | tee "$out/$opt.plain.log"
done
for mutant in group_order owner_order clear_read pointer_width matrix_offset invalid_segment; do
    case "$mutant" in
        group_order) expression='/uint32_t tg =/ { N; s/^\(.*\)\n\(.*\)$/\2\n\1/; }'; suite=retail ;;
        owner_order) expression='/uint32_t tg =/ { N; N; s/^\(.*\)\n\(.*\)\n\(.*\)$/\3\n\1\n\2/; }'; suite=retail ;;
        clear_read) expression='/return value;/i\    if (width == 2) ((uint8_t *)ram->bytes)[at] = 0;'; suite=retail ;;
        pointer_width) expression='s/0x800D3364u, 4/0x800D3364u, 1/'; suite=retail ;;
        matrix_offset) expression='s/0x140u + ag/0x141u + ag/'; suite=retail ;;
        invalid_segment) expression='/static uint32_t offset.*{/a\    return address \& 0x1FFFFF;'; suite=bounds ;;
    esac
    sed "$expression" pc_port/src/battle_target_eligibility_ram.c > "$out/$mutant.c"
    if cmp -s pc_port/src/battle_target_eligibility_ram.c "$out/$mutant.c"; then
        echo "ELIGIBILITY RAM mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -Ipc_port/src -DXBT_TEST_RAM_TRACE -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie "$out/$mutant.o" "$out/O2.cpu.o" "$out/O2.$suite.o" \
        -o "$out/$mutant.test"
    if "$out/$mutant.test" > "$out/$mutant.log" 2>&1; then
        echo "ELIGIBILITY RAM mutant survived: $mutant" >&2; exit 1
    fi
    if [ "$suite" = retail ]; then
        rg -q 'ELIGIBILITY FAIL packed RAM (differs from retail|read trace differs from retail|changed input)' "$out/$mutant.log"
    else
        rg -q 'Assertion.*failed' "$out/$mutant.log"
    fi
    echo "ELIGIBILITY RAM mutant rejected: $mutant"
done
