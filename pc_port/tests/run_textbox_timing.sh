#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${TEXTBOX_TIMING_BUILD_DIR:-$ROOT/pc_port/build_native/textbox_timing}"
SCRATCH="${TEXTBOX_TIMING_SCRATCH:-/tmp/grok-goal-9a828366ad41/implementer}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR" "$SCRATCH"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0 -fno-pie -fno-builtin
      -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src
     -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
SYS=src/slus_006.64/system/system.c
RENDER=src/field/dialogue/text_box_render.c
TEST=pc_port/tests/textbox_timing_prod_test.c

compile_and_run() {
    local name="$1"
    local rc=0
    shift
    "$CC" "${BASE[@]}" -w -include assert.h -include stdint.h \
        "${INC[@]}" "$@" \
        -c "$SYS" -o "$BUILD_DIR/$name.sys.o"
    "$CC" "${BASE[@]}" -w -include assert.h -include stdint.h \
        "${INC[@]}" "$@" \
        -c "$RENDER" -o "$BUILD_DIR/$name.render.o"
    "$CC" "${BASE[@]}" -w -include assert.h -include stdint.h \
        "${INC[@]}" "$@" \
        -c "$TEST" -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie -Wl,--gc-sections "$@" \
        "$BUILD_DIR/$name.sys.o" \
        "$BUILD_DIR/$name.render.o" \
        "$BUILD_DIR/$name.test.o" \
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
    rg -q '^TEXTBOX TIMING CERTIFICATE PASS$' \
        "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
done
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"

cp "$BUILD_DIR/O0.stdout" "$SCRATCH/textbox_timing.stdout"
cp "$BUILD_DIR/O0.stderr" "$SCRATCH/textbox_timing.stderr"
{
    echo '=== O0 ==='
    cat "$BUILD_DIR/O0.stdout"
    echo '=== O2 PASS line ==='
    rg '^TEXTBOX TIMING CERTIFICATE PASS$' "$BUILD_DIR/O2.stdout"
    echo '=== UBSan PASS line ==='
    rg '^TEXTBOX TIMING CERTIFICATE PASS$' "$BUILD_DIR/UBSan.stdout"
} >"$SCRATCH/textbox_timing_all.stdout"

echo 'CERTIFICATE O0/O2/UBSan PASS; TEXTBOX TIMING CERTIFICATE PASS'
