#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B68_BUILD_DIR:-$ROOT/pc_port/build_native/w34b68_second_scheduler}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -ffunction-sections -fdata-sections)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
# Existing unrelated signedness diagnostics in the full 0x800712D0 body are
# outside this focused seam; the test TU and all other diagnostics stay strict.
DRIVER_WARN=(-Wno-sign-conversion -Wno-unused-function)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
SRC=pc_port/src/world_map_frame_driver_712d0.c
TEST=pc_port/tests/w34b68_second_scheduler_prod_test.c

compile_and_run() {
    local name="$1"
    local run_rc=0
    shift
    "$CC" "${BASE[@]}" "${WARN[@]}" "${DRIVER_WARN[@]}" "${INC[@]}" "$@" -c "$SRC" \
        -o "$BUILD_DIR/$name.driver.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" -c "$TEST" \
        -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie -Wl,--gc-sections "$@" "$BUILD_DIR/$name.test.o" \
        "$BUILD_DIR/$name.driver.o" -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr" || run_rc=$?
    return "$run_rc"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2
compile_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^W34B68 second scheduler certificate PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
    nm "$BUILD_DIR/$regime.driver.o" | rg -q ' U wm_80097800$'
done
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"

clear_line="$(rg -n '^[[:space:]]+wm_ot_clear_r_guest\(ot_ptr, 0x400u\);' \
    "$SRC" | cut -d: -f1)"
scheduler_line="$(rg -n '^[[:space:]]+wm_712d0_run_second_scheduler\(\);' \
    "$SRC" | cut -d: -f1)"
draw_line="$(rg -n '^[[:space:]]+\(void\)wm_ot_draw_otag_guest\(guest_ot \+ 0xFFCu\);' \
    "$SRC" | cut -d: -f1)"
test "$clear_line" -lt "$scheduler_line"
test "$scheduler_line" -lt "$draw_line"
echo "ORDER CLEAR_OT -> SECOND_SCHEDULER -> DRAW_OT PASS"

for entry in \
    'M1:WM_712D0_MUTANT_NO_SECOND_SCHEDULER' \
    'M2:WM_712D0_MUTANT_DOUBLE_SECOND_SCHEDULER'; do
    label="${entry%%:*}"
    define="${entry#*:}"
    set +e
    compile_and_run "$label" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! rg -q '^ASSERTION second.scheduler.exactly.once' \
        "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED mutant gate rc=$rc" >&2
        exit 1
    fi
    echo "$label DETECTED"
done

echo "W34B68 CERTIFICATE PASS O0/O2/UBSan; M1-M2 DETECTED; focused warnings clean"
