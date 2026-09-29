#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-radius.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in test cpu body; do
        case "$source" in
            test) file=pc_port/tests/battle_target_radius_test.c ;;
            cpu) file=pc_port/src/battle_mips_adapter.c ;;
            body) file=pc_port/src/battle_target_bounds.c ;;
        esac
        cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src -c "$file" -o "$out/$opt.$source.o"
    done
    clang -no-pie "${flags[@]}" "$out/$opt.test.o" "$out/$opt.cpu.o" "$out/$opt.body.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
for mutant in baseline unsigned_max unsigned_half wrong_origin; do
    case "$mutant" in
        baseline) expression='' ;;
        unsigned_max) expression='s/signed_word(maximum)<signed_word(sum)/maximum<sum/' ;;
        unsigned_half) expression='s/return value<0x8000u?(int32_t)value:(int32_t)value-65536;/return value;/' ;;
        wrong_origin) expression='s/screen_y-164u/screen_y-160u/' ;;
    esac
    sed "$expression" pc_port/src/battle_target_bounds.c > "$out/$mutant.c"
    if [ "$mutant" != baseline ] && cmp -s "$out/$mutant.c" pc_port/src/battle_target_bounds.c; then
        echo "RADIUS mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie "$out/$mutant.o" "$out/O2.cpu.o" "$out/O2.test.o" -o "$out/$mutant.test"
    if [ "$mutant" = baseline ]; then
        "$out/$mutant.test" 2>"$out/$mutant.stderr" | tee "$out/$mutant.log"
        test ! -s "$out/$mutant.stderr"
    else
        if "$out/$mutant.test" >"$out/$mutant.log" 2>&1; then
            echo "RADIUS mutant survived: $mutant" >&2; exit 1
        fi
        rg -q 'Assertion.*actual==cpu.gpr\[21\].*failed' "$out/$mutant.log"
        echo "RADIUS native mutant rejected: $mutant"
    fi
done
cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -DXBT_RADIUS_UNSIGNED_RETAIL \
    -c pc_port/tests/battle_target_radius_test.c -o "$out/unsigned-retail.o"
clang -no-pie "$out/unsigned-retail.o" "$out/O2.cpu.o" "$out/O2.body.o" -o "$out/unsigned-retail.test"
if "$out/unsigned-retail.test" >"$out/unsigned-retail.log" 2>&1; then
    echo 'RADIUS unsigned retail comparison survived' >&2; exit 1
fi
rg -q 'Assertion.*actual==cpu.gpr\[21\].*failed' "$out/unsigned-retail.log"
echo 'RADIUS unsigned retail comparison rejected'
