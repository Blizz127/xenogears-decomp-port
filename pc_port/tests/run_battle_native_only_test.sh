#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source pc_port/tests/lib/sanitizer_preflight.sh
ubsan_preflight clang
OUT=${NATIVE_ONLY_TEST_OUT:-$(mktemp -d /tmp/xeno-native-only.XXXXXXXX)}
mkdir -p "$OUT"
printf 'NATIVE ONLY artifacts=%s\n' "$OUT"
common=(-std=c11 -Wall -Wextra -Werror -Ipc_port/src)
source=pc_port/src/battle_mips_adapter.c
test_source=pc_port/tests/battle_native_only_test.c
gcc "${common[@]}" -DXENO_NATIVE_ONLY -E -P "$source" > "$OUT/native-only.i"
if rg -q '\b(execute_one|mem_read|mem_write|schedule_load)\s*\(' "$OUT/native-only.i"; then
    echo 'NATIVE ONLY FAIL CPU execution code remains after preprocessing' >&2
    exit 1
fi
for mode in O0 O2 UBSan; do
    compiler=gcc
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then
        compiler=clang
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    "$compiler" "${common[@]}" "${flags[@]}" -DXENO_NATIVE_ONLY -c "$source" -o "$OUT/$mode.cpu.o"
    "$compiler" "${common[@]}" "${flags[@]}" "$test_source" "$OUT/$mode.cpu.o" -o "$OUT/$mode.test"
    "$OUT/$mode.test" > "$OUT/$mode.log" 2>&1
    cat "$OUT/$mode.log"
done
# The ordinary hybrid adapter must fail this exact test with the switch unset.
gcc "${common[@]}" -O0 "$test_source" "$source" -o "$OUT/interpreter-present.test"
status=0
"$OUT/interpreter-present.test" > "$OUT/interpreter-present.log" 2>&1 || status=$?
if [ "$status" != 1 ] || ! rg -q 'NATIVE ONLY FAIL runtime configuration enabled interpreter' "$OUT/interpreter-present.log"; then
    echo 'NATIVE ONLY FAIL interpreter-present control survived' >&2
    exit 1
fi
echo 'NATIVE ONLY interpreter-present control rejected'
