#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${RESULTS_TRACE_TEST_OUT:?set a private disk output directory}"
mkdir -p "$RESULTS_TRACE_TEST_OUT"
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    clang -std=gnu17 -Wall -Wextra -Werror "${flags[@]}" -Ipc_port/src \
        pc_port/tests/battle_results_trace_test.c pc_port/src/battle_results_trace.c \
        -o "$RESULTS_TRACE_TEST_OUT/$mode"
    for control in off native fault; do
        mkdir -p "$RESULTS_TRACE_TEST_OUT/$mode-$control"
        "$RESULTS_TRACE_TEST_OUT/$mode" "$control" "$RESULTS_TRACE_TEST_OUT/$mode-$control"
    done
done
