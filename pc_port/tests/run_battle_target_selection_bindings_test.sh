#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-selection-bindings.XXXXXX)
echo "Evidence: $out"
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=gnu17 "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/battle_target_selection_bindings_test.c -o "$out/$opt.o"
    clang -no-pie "${flags[@]}" "$out/$opt.o" -o "$out/$opt.test"
    "$out/$opt.test" | tee "$out/$opt.log"
done
