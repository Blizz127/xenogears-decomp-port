#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B22_I4B_BUILD_DIR:-$ROOT/pc_port/build_native/w34b22_i4b}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DWM_PRIVATE_PROBE_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34b22_i4b_private_family_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_private_collision.c)

build_and_run() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" 2>"$BUILD_DIR/$name.stderr"
    rg -v 'PSX RAM emulation' "$BUILD_DIR/$name.raw" >"$BUILD_DIR/$name.stdout" || true
}

build_and_run focused_O0 -O0 -g
build_and_run focused_O2 -O2
build_and_run focused_ubsan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
for name in focused_O0 focused_O2 focused_ubsan; do
    rg -q '^W34B22-I4B private collision family PASS cases=20 wrappers=4$' "$BUILD_DIR/$name.stdout"
    test ! -s "$BUILD_DIR/$name.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS; normalized output identical"

mutants=(
    SWAP_RATIO UNSIGNED_ARITH WRONG_SHIFT_TRUNC MISSING_TRAP WRONG_MASK
    WRONG_EDGE_SOURCE WRONG_MINUS_ONE WRONG_SEED WRONG_FIELD SWAP_AXIS_ORTHO
    WRONG_CALL_ORDER WRONG_CANDIDATE_PTR SWAP_94060_ARGS NO_MODE_SIGNEXT
    NO_CODE_SIGNEXT WRONG_FIRST_BRANCH NO_SHORT_CIRCUIT WRONG_COPY
    BOOLEAN_RETURN WRONG_X_CODE WRONG_Z_CODE
)
killed=0
for mutant in "${mutants[@]}"; do
    name="mutant_$mutant"
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 \
        -DWM_PRIVATE_PROBE_MUTANT_BUILD -DWM_PRIVATE_MUTANT_"$mutant" \
        "${SRC[@]}" -o "$BUILD_DIR/$name"
    set +e
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr"
    rc=$?
    set -e
    if [ "$rc" -eq 0 ] || ! rg -q '^ASSERTION ' "$BUILD_DIR/$name.stderr"; then
        echo "$mutant FAILED mutant gate rc=$rc" >&2
        tail -30 "$BUILD_DIR/$name.stderr" >&2 || true
        exit 1
    fi
    echo "$mutant KILLED rc=$rc $(rg -m1 '^ASSERTION ' "$BUILD_DIR/$name.stderr")"
    killed=$((killed + 1))
done
echo "MUTANTS $killed/${#mutants[@]} KILLED"

gcc -std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
    -DWM_93354_TEST_TRACE -DWM_93E8C_TEST_TRACE -DWM_93F18_TEST_TRACE \
    -DWM_94060_TEST_TRACE "${WARN[@]}" "${INC[@]}" -O2 \
    pc_port/tests/w34b22_i4b_scratch_integration_test.c \
    pc_port/src/psx_memory.c pc_port/src/world_map_helper_93354.c \
    pc_port/src/world_map_helper_93e8c.c pc_port/src/world_map_helper_93f18.c \
    pc_port/src/world_map_helper_94060.c \
    pc_port/src/world_map_private_collision.c -o "$BUILD_DIR/scratch_integration"
"$BUILD_DIR/scratch_integration" >"$BUILD_DIR/scratch_integration.raw" \
    2>"$BUILD_DIR/scratch_integration.stderr"
rg -v 'PSX RAM emulation' "$BUILD_DIR/scratch_integration.raw" \
    >"$BUILD_DIR/scratch_integration.stdout" || true
rg -q '^W34B22-I4B scratchpad canonical-chain integration PASS$' \
    "$BUILD_DIR/scratch_integration.stdout"
test ! -s "$BUILD_DIR/scratch_integration.stderr"
echo "SCRATCHPAD CANONICAL-CHAIN INTEGRATION PASS"
echo "W34B22-I4B private collision family certificate PASS"
