#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34C1_RUNG3A_BUILD_DIR:-$ROOT/pc_port/build_native/w34c1_rung3a_controller}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -ffunction-sections -fdata-sections)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
# The full driver has pre-existing signedness diagnostics outside this seam.
# Keep the certificate TU strict and suppress only those legacy driver-TU
# diagnostics, matching the established focused-certificate policy.
DRIVER_WARN=(-Wno-sign-conversion -Wno-unused-function)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
SRC=pc_port/src/world_map_frame_driver_712d0.c
TEST=pc_port/tests/w34c1_world_controller_sources_prod_test.c

compile_and_run() {
    local name="$1"
    local source="$2"
    local run_rc=0
    shift 2
    "$CC" "${BASE[@]}" "${WARN[@]}" "${DRIVER_WARN[@]}" "${INC[@]}" \
        "$@" -c "$source" -o "$BUILD_DIR/$name.driver.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" -c "$TEST" \
        -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie -Wl,--gc-sections "$@" "$BUILD_DIR/$name.test.o" \
        "$BUILD_DIR/$name.driver.o" -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr" || run_rc=$?
    return "$run_rc"
}

assert_retail_source_order() {
    local previous=0
    local symbol
    local line
    local -a symbols=(
        g_C1ButtonState
        g_C2ButtonState
        g_C1ButtonStateReleased
        g_C2ButtonStateReleased
        g_C1ButtonStatePressedOnce
        g_C2ButtonStatePressedOnce
    )

    for symbol in "${symbols[@]}"; do
        line="$(rg -n "^[[:space:]]+.*\\(u16\\)$symbol;" "$SRC" | cut -d: -f1)"
        test -n "$line"
        test "$line" -gt "$previous"
        previous="$line"
    done
    if rg -q '0x80069(570|574|48C|490|4A4|4A8)' "$SRC"; then
        echo "ASSERTION controller.source.no_plus_10000_guest_addresses" >&2
        exit 1
    fi
    echo "ORDER C1_HELD -> C2_HELD -> C1_RELEASED -> C2_RELEASED -> C1_PRESSED_ONCE -> C2_PRESSED_ONCE PASS"
}

assert_retail_source_order

compile_and_run O0 "$SRC" -O0 -g
compile_and_run O2 "$SRC" -O2
compile_and_run UBSan "$SRC" -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    rg -q '^W34C1 RUNG3A controller source certificate PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"

make_mutant() {
    local name="$1"
    cp "$SRC" "$BUILD_DIR/$name.c"
}

make_mutant M1
perl -0pi -e 's/\(u16\)g_C1ButtonState;/fd_lhu\(0x80069570u\);/' \
    "$BUILD_DIR/M1.c"
rg -q 'raw_btn = fd_lhu\(0x80069570u\);' "$BUILD_DIR/M1.c"

make_mutant M2
perl -0pi -e 's/\(u16\)g_C2ButtonStateReleased;/0u;/' "$BUILD_DIR/M2.c"
rg -q 'raw_stick = 0u;' "$BUILD_DIR/M2.c"

make_mutant M3
perl -0pi -e 's/(fd_sh\(D_8009BD10, 0\);\n)[[:space:]]+fd_sh\(D_8009CD4C, 0\);/$1/' \
    "$BUILD_DIR/M3.c"
if rg -q 'fd_sh\(D_8009CD4C, 0\);' "$BUILD_DIR/M3.c"; then
    echo "M3 failed to remove the sole recurring-frame clear" >&2
    exit 1
fi

for entry in \
    'M1:controller.source.c1_held.native_not_guest_plus_10000' \
    'M2:controller.source.c2_released.accumulates_bd14' \
    'M3:controller.accumulator.cd4c.cleared_before_drain'; do
    label="${entry%%:*}"
    assertion="${entry#*:}"
    set +e
    compile_and_run "$label" "$BUILD_DIR/$label.c" -O0 -g
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || \
       ! rg -q "^ASSERTION $assertion([[:space:]]|$)" \
           "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED mutant gate rc=$rc expected=$assertion" >&2
        exit 1
    fi
    echo "$label DETECTED assertion=$assertion"
done

echo "W34C1 RUNG3A CERTIFICATE PASS O0/O2/UBSan; M1-M3 DETECTED; focused warnings clean"
