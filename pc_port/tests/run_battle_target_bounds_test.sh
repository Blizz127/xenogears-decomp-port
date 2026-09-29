#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-bounds.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in test cpu body; do
        case "$source" in
            test) file=pc_port/tests/battle_target_bounds_test.c ;;
            cpu) file=pc_port/src/battle_mips_adapter.c ;;
            body) file=pc_port/src/battle_target_bounds.c ;;
        esac
        cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src -c "$file" -o "$out/$opt.$source.o"
    done
    clang -no-pie "${flags[@]}" "$out/$opt.test.o" "$out/$opt.cpu.o" "$out/$opt.body.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
for mutant in baseline logical_shift ignore_flag ten_slots true_extrema; do
    case "$mutant" in
        baseline) expression='' ;;
        logical_shift) expression='s/return (value>>shift)|((0u-(value>>31))<<(32-shift));/return value>>shift;/' ;;
        ignore_flag) expression='s/points\[i\].suppressed || //g' ;;
        ten_slots) expression='s/i<11/i<10/g' ;;
        true_extrema) expression='s/low\[axis\]=high\[axis\]=(uint32_t)(signed_word(sums\[axis\])\/result->count)<<1;/low[axis]=0x7FFFFFFFu, high[axis]=0x80000000u;/' ;;
    esac
    sed "$expression" pc_port/src/battle_target_bounds.c > "$out/$mutant.c"
    if [ "$mutant" != baseline ] && cmp -s "$out/$mutant.c" pc_port/src/battle_target_bounds.c; then
        echo "BOUNDS mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie "$out/$mutant.o" "$out/O2.cpu.o" "$out/O2.test.o" -o "$out/$mutant.test"
    if [ "$mutant" = baseline ]; then
        "$out/$mutant.test" 2>"$out/$mutant.stderr" | tee "$out/$mutant.log"
        test ! -s "$out/$mutant.stderr"
    else
        if "$out/$mutant.test" >"$out/$mutant.log" 2>&1; then
            echo "BOUNDS mutant survived: $mutant" >&2; exit 1
        fi
        case "$mutant" in
            logical_shift|true_extrema) pattern='Assertion.*result.center\[axis\].*failed' ;;
            ignore_flag|ten_slots) pattern='Assertion.*result.count==count.*failed' ;;
        esac
        rg -q "$pattern" "$out/$mutant.log"
        echo "BOUNDS native mutant rejected: $mutant"
    fi
done
