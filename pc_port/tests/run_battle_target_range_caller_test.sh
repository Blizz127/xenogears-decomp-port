#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-range-caller.XXXXXX)
echo "Evidence: $out"
read -r actual _ < <(sha256sum disc/battle.bin)
test "$actual" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/tests/battle_target_range_caller_test.c -o "$out/$opt.test.o"
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror -Ipc_port/src \
        -c pc_port/src/battle_mips_adapter.c -o "$out/$opt.cpu.o"
    clang -no-pie "${flags[@]}" "$out/$opt.test.o" "$out/$opt.cpu.o" -o "$out/$opt.test"
    "$out/$opt.test" 2>"$out/$opt.stderr" | tee "$out/$opt.log"
    test ! -s "$out/$opt.stderr"
done
cc -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src -DXBT_CALLER_BAD_MODE \
    pc_port/tests/battle_target_range_caller_test.c pc_port/src/battle_mips_adapter.c \
    -o "$out/bad-mode.test"
if "$out/bad-mode.test" >"$out/bad-mode.log" 2>&1; then
    echo 'CALLER mode mutant survived' >&2; exit 1
fi
rg -q 'Assertion.*cpu.gpr\[4\]==mode.*failed' "$out/bad-mode.log"
echo 'CALLER out-of-domain mode mutant rejected'
