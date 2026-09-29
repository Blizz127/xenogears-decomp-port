#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34C1_BUILD_DIR:-$ROOT/pc_port/build_native/w34c1_animation_guard}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -fno-pie -no-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src)
SRC=(pc_port/tests/w34c1_animation_guard_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_callback_8b644.c
     pc_port/src/world_map_callback_8c844.c
     pc_port/src/world_map_callback_8d678.c)

check_sites() {
    local count
    count="$(rg -o 'wm_native_animation_(differs|value)\(' \
        pc_port/src/world_map_callback_{8a72c,8b644,8c844,8d678}.c | wc -l)"
    test "$count" -eq 11
    if rg -n 'PSX_ADDR\([^\n]*(actor|obj)|PSX_ADDR\([^\n]*\+ 0xAF' \
        pc_port/src/world_map_callback_{8a72c,8b644,8c844,8d678}.c; then
        echo "ASSERTION source-inventory: legacy PSX_ADDR animation guard remains" >&2
        exit 1
    fi
    echo "SOURCE_INVENTORY=11_NATIVE_POINTER_GUARDS"
}

build_and_run() {
    local name="$1"
    shift
    gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
    rg -q '^W34C1 animation guard certificate PASS$' \
        "$BUILD_DIR/$name.stdout"
    test ! -s "$BUILD_DIR/$name.stderr"
    rg -v '^\[xeno-port\] PSX RAM emulation:' "$BUILD_DIR/$name.stdout" \
        >"$BUILD_DIR/$name.normalized"
}

check_sites
build_and_run focused_O0 -O0 -g
build_and_run focused_O2 -O2
build_and_run focused_ubsan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
cmp "$BUILD_DIR/focused_O0.normalized" "$BUILD_DIR/focused_O2.normalized"
cmp "$BUILD_DIR/focused_O0.normalized" "$BUILD_DIR/focused_ubsan.normalized"
echo "FOCUSED O0/O2/UBSAN PASS; strict warnings clean"

set +e
gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 \
    -DWM_ANIMATION_GUARD_MUTANT_GUEST_REMAP "${SRC[@]}" \
    -o "$BUILD_DIR/mutant_guest_remap"
compile_rc=$?
if [ "$compile_rc" -eq 0 ]; then
    "$BUILD_DIR/mutant_guest_remap" >"$BUILD_DIR/mutant_guest_remap.stdout" \
        2>"$BUILD_DIR/mutant_guest_remap.stderr"
    mutant_rc=$?
else
    mutant_rc=$compile_rc
fi
set -e
if [ "$compile_rc" -ne 0 ] || [ "$mutant_rc" -eq 0 ] || \
   ! rg -q '^ASSERTION (8b644|8c844|8d678)\.' \
       "$BUILD_DIR/mutant_guest_remap.stderr"; then
    echo "WM_ANIMATION_GUARD_MUTANT_GUEST_REMAP FAILED named gate" >&2
    exit 1
fi
echo "WM_ANIMATION_GUARD_MUTANT_GUEST_REMAP DETECTED"
echo "W34C1 RUNG1 ANIMATION GUARD CERTIFICATE PASS"
