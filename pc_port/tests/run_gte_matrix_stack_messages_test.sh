#!/usr/bin/env bash
# Run against the current patched SDK, with no disc or graphics context.
set -euo pipefail
cd "$(dirname "$0")/../.."
read -r -a test_cc <<< "${CC:-cc}"
read -r -a test_cxx <<< "${CXX:-c++}"
task_stack_tmp=$(mktemp -d)
trap 'rm -rf "$task_stack_tmp"' EXIT
psx=pc_port/extern/PsyCross
inc=(-DUSE_EXTENDED_PRIM_POINTERS=0 -include pc_port/src/port_compat.h
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -I"$psx/include" -I"$psx/include/psx")
for mode in ${GTE_STACK_TEST_MODES:-O0 O2}; do
    flags=(-"$mode" -ffunction-sections -fdata-sections)
    "${test_cc[@]}" -std=gnu17 "${flags[@]}" "${inc[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/gte_matrix_stack_messages_test.c -o "$task_stack_tmp/test.o"
    for src in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        "${test_cxx[@]}" -std=gnu++17 "${flags[@]}" "${inc[@]}" -Wno-narrowing \
            -c "$psx/src/$src" -o "$task_stack_tmp/$(basename "$src").o"
    done
    "${test_cxx[@]}" "${flags[@]}" -Wl,--gc-sections \
        "$task_stack_tmp/test.o" "$task_stack_tmp/LIBGTE.C.o" \
        "$task_stack_tmp/INLINE_C.C.o" "$task_stack_tmp/PsyX_GTE.cpp.o" \
        -o "$task_stack_tmp/test"
    "$task_stack_tmp/test"
done
