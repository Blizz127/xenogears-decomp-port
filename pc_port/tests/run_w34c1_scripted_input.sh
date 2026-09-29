#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34C1_INPUT_BUILD_DIR:-$ROOT/pc_port/build_native/w34c1_scripted_input}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DXENO_TEST_INPUT_CERTIFICATE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34c1_scripted_input_prod_test.c
     pc_port/src/test_input.c)

test "$(rg -c 'PcPort_TestInputAdvanceFrame\(\);' \
    src/field/main/misc2.c)" -eq 1
test "$(rg -c 'PcPort_TestInputInject\(&D_800AFE9C\);' \
    src/field/main/misc6.c)" -eq 1
test "$(rg -c 'PcPort_TestInput(AdvanceFrame\(\)|Inject\(&D_800AFE9C\))' \
    pc_port/src/world_map_main_loop_71034.c)" -eq 2
test "$(rg -c 'PcPort_TestInputInit\(\)' pc_port/src/port_main.c)" -eq 1
test "$(rg -c 'PcPort_WorldTestInputInit\(\)' pc_port/src/port_main.c)" -eq 1
test "$(rg -c 'PcPort_WorldTestInputMerge\(' \
    pc_port/src/world_map_frame_driver_712d0.c)" -eq 2
echo "SOURCE_INVENTORY=STARTUP_PARSE+FIELD_CLOCK+FIELD_INJECT+WORLD_CONTINUATION"

compile_and_run() {
    local name="$1"
    local run_rc=0
    shift
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr" || run_rc=$?
    return "$run_rc"
}

for regime in O0 O2 UBSan; do
    flags=(-O0 -g)
    if [[ "$regime" == O2 ]]; then flags=(-O2); fi
    if [[ "$regime" == UBSan ]]; then
        flags=(-O2 -g -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    compile_and_run "$regime" "${flags[@]}"
    rg -q '^W34C1 scripted input certificate PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test "$(rg -c '^\[test-input\] invalid XENO_TEST_INPUT:' \
        "$BUILD_DIR/$regime.stderr")" -eq 7
    test "$(rg -c '^\[world-test-input\] invalid XENO_WORLD_TEST_INPUT:' \
        "$BUILD_DIR/$regime.stderr")" -eq 1
    rg -v '^\[(test-input|world-test-input)\] (invalid XENO_(WORLD_)?TEST_INPUT:|enabled steps=|frame=)' \
        "$BUILD_DIR/$regime.stderr" >"$BUILD_DIR/$regime.unexpected" || true
    test ! -s "$BUILD_DIR/$regime.unexpected"
done
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
echo "W34C1 SCRIPTED INPUT O0/O2/UBSAN PASS; strict warnings clean"

set +e
compile_and_run mutant_drop_hold -O0 -g \
    -DXENO_TEST_INPUT_MUTANT_DROP_HOLD
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION hold.frame1$' "$BUILD_DIR/mutant_drop_hold.stderr"; then
    echo "XENO_TEST_INPUT_MUTANT_DROP_HOLD FAILED named gate" >&2
    exit 1
fi
echo "XENO_TEST_INPUT_MUTANT_DROP_HOLD DETECTED"

set +e
compile_and_run mutant_no_world_rising -O0 -g \
    -DXENO_WORLD_TEST_INPUT_MUTANT_NO_RISING
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION world.frame1.rising$' \
       "$BUILD_DIR/mutant_no_world_rising.stderr"; then
    echo "XENO_WORLD_TEST_INPUT_MUTANT_NO_RISING FAILED named gate" >&2
    exit 1
fi
echo "XENO_WORLD_TEST_INPUT_MUTANT_NO_RISING DETECTED"
echo "W34C1 RUNG2 SCRIPTED INPUT CERTIFICATE PASS"
