#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-setup.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_setup_contract_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    clang -no-pie "${flags[@]}" "$out/$opt.test.o" "$out/$opt.cpu.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -DXBT_SETUP_NATIVE -c pc_port/tests/battle_target_setup_contract_test.c -o "$out/$opt.native-test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_target_setup.c -o "$out/$opt.native.o"
    clang -no-pie "${flags[@]}" "$out/$opt.native-test.o" "$out/$opt.native.o" \
        "$out/$opt.cpu.o" -o "$out/$opt.native.test"
    "$out/$opt.native.test" 2>"$out/$opt.native.stderr" | tee "$out/$opt.native.log"
    test ! -s "$out/$opt.native.stderr"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_setup_failure_test.c -o "$out/$opt.failure.o"
    clang -no-pie "${flags[@]}" "$out/$opt.failure.o" "$out/$opt.native.o" -o "$out/$opt.failure.test"
    "$out/$opt.failure.test" 2>"$out/$opt.failure.stderr" | tee "$out/$opt.failure.log"
    test ! -s "$out/$opt.failure.stderr"
done
cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -DXBT_SETUP_NO_FIRST_CLEAR \
    -c pc_port/tests/battle_target_setup_contract_test.c -o "$out/no-clear.o"
clang -no-pie "$out/no-clear.o" "$out/O2.cpu.o" -o "$out/no-clear.test"
if "$out/no-clear.test" >"$out/no-clear.log" 2>&1; then
    echo 'SETUP no-clear mutant survived' >&2; exit 1
fi
rg -q 'Assertion.*(get\(0xC3680,4\)==0|writes==4\+calls).*failed' "$out/no-clear.log"
echo 'SETUP missing post-callback root clear rejected'
for mutant in baseline mask_task wrong_clear narrow_mode; do
    case "$mutant" in
        baseline) expression='' ;;
        mask_task) expression='s/task + 12/(task \& 0x1FFFFFFFu) + 12/' ;;
        wrong_clear) expression='s/address, 4, 0/address, 4, 1/' ;;
        narrow_mode) expression='s/mode == 2/(mode \& 255) == 2/' ;;
    esac
    sed "$expression" pc_port/src/battle_target_setup.c > "$out/$mutant.c"
    if [ "$mutant" != baseline ] && cmp -s pc_port/src/battle_target_setup.c "$out/$mutant.c"; then
        echo "SETUP mutation made no change: $mutant" >&2; exit 1
    fi
    cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -c "$out/$mutant.c" -o "$out/$mutant.o"
    clang -no-pie "$out/$mutant.o" "$out/O2.native-test.o" "$out/O2.cpu.o" -o "$out/$mutant.test"
    if [ "$mutant" = baseline ]; then
        "$out/$mutant.test" 2>"$out/$mutant.stderr" | tee "$out/$mutant.log"
        test ! -s "$out/$mutant.stderr"
    else
        if "$out/$mutant.test" >"$out/$mutant.log" 2>&1; then
            echo "SETUP mutant survived: $mutant" >&2; exit 1
        fi
        case "$mutant" in
            mask_task) pattern='Assertion.*address>=0x80000000u.*failed' ;;
            wrong_clear) pattern='Assertion.*get\(0xC368[04],4\)==0.*failed' ;;
            narrow_mode) pattern='Assertion.*calls==expected_calls.*failed' ;;
        esac
        rg -q "$pattern" "$out/$mutant.log"
        echo "SETUP native mutant rejected: $mutant"
    fi
done
