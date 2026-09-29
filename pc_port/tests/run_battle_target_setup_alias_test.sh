#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-setup-alias.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in test cpu body; do
        case "$source" in
            test) file=pc_port/tests/battle_target_setup_alias_test.c ;;
            cpu) file=pc_port/src/battle_mips_adapter.c ;;
            body) file=pc_port/src/battle_target_setup.c ;;
        esac
        cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src -c "$file" -o "$out/$opt.$source.o"
    done
    clang -no-pie "${flags[@]}" "$out/$opt.test.o" "$out/$opt.cpu.o" "$out/$opt.body.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
for reg in 16 31; do
    sed "s/return setup(\&cpu->bus,cpu->gpr\[4\],frame_invoke,\&call,cpu);/uint32_t saved=cpu->gpr[$reg]; int result=setup(\&cpu->bus,cpu->gpr[4],frame_invoke,\&call,cpu); cpu->gpr[$reg]=saved; return result;/" \
        pc_port/src/battle_target_setup.c > "$out/cached-$reg.c"
    if cmp -s "$out/cached-$reg.c" pc_port/src/battle_target_setup.c; then
        echo 'ALIAS mutation did not apply' >&2; exit 1
    fi
    cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -c "$out/cached-$reg.c" -o "$out/cached-$reg.o"
    clang -no-pie "$out/cached-$reg.o" "$out/O2.cpu.o" "$out/O2.test.o" -o "$out/cached-$reg.test"
    if "$out/cached-$reg.test" >"$out/cached-$reg.log" 2>&1; then
        echo "ALIAS cached register mutant survived: $reg" >&2; exit 1
    fi
    rg -q 'Assertion.*native.gpr\[(r|31)\]==.*failed' "$out/cached-$reg.log"
    echo "ALIAS cached register mutant rejected: $reg"
done
