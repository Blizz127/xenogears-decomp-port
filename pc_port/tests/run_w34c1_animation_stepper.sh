#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34C1_STEPPER_BUILD_DIR:-$ROOT/pc_port/build_native/w34c1_animation_stepper}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -fno-pie -no-pie -DXENO_PC_PORT -DSKIP_ASM \
      -D_LANGUAGE_C -DWM_85CDC_CERTIFICATE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include \
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
SRC=(pc_port/tests/w34c1_animation_stepper_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_85cdc.c)

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
    rg -q '^W34C1 animation stepper certificate PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
    rg -v '^\[xeno-port\] PSX RAM emulation:' "$BUILD_DIR/$regime.stdout" \
        >"$BUILD_DIR/$regime.normalized" || true
done
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/O2.normalized"
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/UBSan.normalized"
echo "W34C1 STEPPER O0/O2/UBSAN PASS; strict warnings clean"

mutants=(
    'M1:WM_85CDC_MUTANT_NO_ANIMATION_TICK:animation_tick.once_per_eligible_object'
    'M2:WM_85CDC_MUTANT_WRONG_DEPTH_GATE:render.signed_depth_gate'
    'M3:WM_85CDC_MUTANT_WRONG_HEADING_CLAMP:heading.positive_step_0x100'
    'M4:WM_85CDC_MUTANT_WRONG_OT_BUCKET:ot_bucket.depth_shift_then_word_scale'
    'M5:WM_85CDC_MUTANT_WRONG_DIRECTION:direction.subtract_global_and_quarter_turn'
)
for entry in "${mutants[@]}"; do
    label="${entry%%:*}"
    rest="${entry#*:}"
    define="${rest%%:*}"
    assertion="${rest#*:}"
    set +e
    compile_and_run "$label" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || \
       ! rg -q "^ASSERTION ${assertion}$" "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED named mutant gate rc=$rc" >&2
        exit 1
    fi
    echo "$label DETECTED"
done
echo "W34C1 RUNG1B ANIMATION STEPPER CERTIFICATE PASS; M1-M5 DETECTED"
