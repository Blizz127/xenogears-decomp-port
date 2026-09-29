#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34N51_BUILD_DIR:-$ROOT/pc_port/build_native/w34n51_font_outline}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0 -fno-pie -fno-builtin
      -fpermissive -ffunction-sections -fdata-sections)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src
     -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
PROD=src/slus_006.64/system/system.c
TEST=pc_port/tests/w34n51_font_outline_prod_test.c

compile_and_run() {
    local name="$1"
    local rc=0
    shift
    "$CC" "${BASE[@]}" -w -include assert.h -include stdint.h \
        "${INC[@]}" "$@" \
        -c "$PROD" -o "$BUILD_DIR/$name.prod.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" \
        -c "$TEST" -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie -Wl,--gc-sections "$@" \
        "$BUILD_DIR/$name.prod.o" "$BUILD_DIR/$name.test.o" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr" || rc=$?
    return "$rc"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all

for regime in O0 O2 UBSan; do
    rg -q '^W34N51 FONT OUTLINE CERTIFICATE PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
echo 'CERTIFICATE O0/O2/UBSan PASS; focused test warnings clean; legacy production TU warning-suppressed'

mutants=(
    'M1:WM_34FFC_MUTANT_REVERSE_ROW0_OUTLINE:row0.empty-glyph-does-not-fill-outline'
    'M2:WM_34FFC_MUTANT_REVERSE_ROW1_OUTLINE:row1.empty-glyph-does-not-fill-outline'
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
    if [[ "$rc" -eq 0 ]] ||
       ! rg -q "^ASSERTION $assertion$" "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED mutant gate assertion=$assertion rc=$rc" >&2
        exit 1
    fi
    echo "$label DETECTED; ASSERTION $assertion"
done

echo 'W34N51 font outline certificate PASS; M1-M2 DETECTED'
