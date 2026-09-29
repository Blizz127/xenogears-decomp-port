#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-cleanup.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_cleanup_retail_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    clang -no-pie "${flags[@]}" "$out/$opt.test.o" "$out/$opt.cpu.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src -DXBT_CLEANUP_NATIVE \
        -c pc_port/tests/battle_target_cleanup_retail_test.c -o "$out/$opt.native-test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_target_setup.c -o "$out/$opt.native.o"
    clang -no-pie "${flags[@]}" "$out/$opt.native-test.o" "$out/$opt.cpu.o" \
        "$out/$opt.native.o" -o "$out/$opt.native.test"
    "$out/$opt.native.test" 2>"$out/$opt.native.stderr" | tee "$out/$opt.native.log"
    test ! -s "$out/$opt.native.stderr"
done
for mutant in NO_REENTRY NO_ROOT_CLEAR EARLY_REENTRY; do
    cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -DXBT_CLEANUP_"$mutant" \
        -c pc_port/tests/battle_target_cleanup_retail_test.c -o "$out/$mutant.o"
    clang -no-pie "$out/$mutant.o" "$out/O2.cpu.o" -o "$out/$mutant.test"
    if "$out/$mutant.test" >"$out/$mutant.log" 2>&1; then
        echo "CLEANUP mutant survived: $mutant" >&2; exit 1
    fi
    case "$mutant" in
        NO_REENTRY) pattern='Assertion.*stack_writes==2\+2\*n\+2\*reentered.*failed' ;;
        NO_ROOT_CLEAR) pattern='Assertion.*get\(0xC3680\+index\*4,4\)==0.*failed' ;;
        EARLY_REENTRY) pattern='Assertion.*phase==0 && freed==initial_count.*failed' ;;
    esac
    rg -q "$pattern" "$out/$mutant.log"
    echo "CLEANUP mutant rejected: $mutant"
done
for mutant in baseline wrong_return wrong_saved_s0; do
    case "$mutant" in
        baseline) expression='' ;;
        wrong_return) expression='s/0x800BC3E0u : 0x800BC3B8u/0x800BC3E4u : 0x800BC3BCu/' ;;
        wrong_saved_s0) expression='s/4, cpu->gpr\[16\]))/4, cpu->gpr[16] ^ 1u))/' ;;
    esac
    sed "$expression" pc_port/src/battle_target_setup.c > "$out/$mutant.c"
    if [ "$mutant" != baseline ] && cmp -s "$out/$mutant.c" pc_port/src/battle_target_setup.c; then
        echo "CLEANUP native mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -c "$out/$mutant.c" -o "$out/$mutant.native.o"
    clang -no-pie "$out/$mutant.native.o" "$out/O2.cpu.o" "$out/O2.native-test.o" -o "$out/$mutant.native.test"
    if [ "$mutant" = baseline ]; then
        "$out/$mutant.native.test" 2>"$out/$mutant.native.stderr" | tee "$out/$mutant.native.log"
        test ! -s "$out/$mutant.native.stderr"
    else
        if "$out/$mutant.native.test" >"$out/$mutant.native.log" 2>&1; then
            echo "CLEANUP native mutant survived: $mutant" >&2; exit 1
        fi
        case "$mutant" in
            wrong_return) pattern='Assertion.*memcmp\(expected_ram.*failed' ;;
            wrong_saved_s0) pattern='Assertion.*cpu.gpr\[i\]==0xCAFE0000\+i.*failed' ;;
        esac
        rg -q "$pattern" "$out/$mutant.native.log"
        echo "CLEANUP native mutant rejected: $mutant"
    fi
done
