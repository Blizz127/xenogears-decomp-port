#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_ACB90_BUILD_DIR:-$ROOT/pc_port/build_native/field_acb90}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

build_and_run() {
    local name="$1"
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" -w "$@" \
        -c src/field/main/misc9.c -o "$BUILD_DIR/$name.misc9.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/field_acb90_retail_test.c -o "$BUILD_DIR/$name.test.o"
    "$CC" -fno-pie -no-pie "$@" "$BUILD_DIR/$name.test.o" \
        "$BUILD_DIR/$name.misc9.o" -Wl,--gc-sections -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O1 -g -fsanitize=undefined -fno-sanitize-recover=all
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
rg -q '^FIELD ACB90 PASS checks=[0-9]+$' "$BUILD_DIR/O0.stdout"
echo "FIELD ACB90 O0/O2/UBSAN PASS"

set +e
build_and_run mutant_old_y -O0 -g -DFIELD_ACB90_MUTANT_OLD_Y
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || ! rg -q '^ASSERTION acb90.tim_y$' \
    "$BUILD_DIR/mutant_old_y.stderr"; then
    echo "OLD-Y MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_old_y.stderr" >&2 || true
    exit 1
fi
echo "FIELD ACB90 OLD-Y MUTANT DETECTED"

set +e
build_and_run mutant_bad_clear -O0 -g -DFIELD_ACB90_MUTANT_BAD_CLEAR_RANGE
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || ! rg -q '^ASSERTION acb90.clear_buffer_word$' \
    "$BUILD_DIR/mutant_bad_clear.stderr"; then
    echo "BAD-CLEAR-RANGE MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_bad_clear.stderr" >&2 || true
    exit 1
fi
echo "FIELD ACB90 BAD-CLEAR-RANGE MUTANT DETECTED"
