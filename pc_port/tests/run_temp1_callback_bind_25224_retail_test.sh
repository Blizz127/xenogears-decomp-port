#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${TEMP1_CALLBACK_BIND_25224_BUILD_DIR:-$ROOT/pc_port/build_native/temp1_callback_bind_25224}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
FN_SHA="$(dd if=disc/SLUS_006.64 bs=1 \
    skip=$((0x800 + 0x80025224 - 0x80010000)) count=$((0x34)) status=none \
    | sha256sum | awk '{print $1}')"
actual="$(sha256sum disc/SLUS_006.64 | awk '{print $1}')"
if [[ "$actual" != "$SLUS_SHA" ]]; then
    echo "ERROR: disc/SLUS_006.64 SHA-256 mismatch: $actual" >&2
    exit 1
fi
if [[ -z "$FN_SHA" ]]; then
    echo "ERROR: could not hash the retail func_80025224 slice" >&2
    exit 1
fi
echo "retail func_80025224 slice sha256: $FN_SHA"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

build_and_run() {
    local name="$1"
    local linker="$CC"
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" -w -fno-inline "$@" \
        -fno-sanitize=object-size \
        -c src/slus_006.64/system/temp1e.c -o "$BUILD_DIR/$name.temp1.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/temp1_callback_bind_25224_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -fno-pie -no-pie "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.temp1.o" \
        -Wl,--gc-sections -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
rg -q '^TEMP1 CALLBACK BIND 25224 certificate PASS checks=[0-9]+$' \
    "$BUILD_DIR/O0.stdout"

echo "TEMP1 CALLBACK BIND 25224 O0/O2/UBSAN PASS"

set +e
build_and_run mutant_index_plus1 -O0 -g -DTEMP1_25224_MUTANT_INDEX_PLUS1
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION temp1.callback ' \
       "$BUILD_DIR/mutant_index_plus1.stderr"; then
    echo "INDEX-PLUS1 MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_index_plus1.stderr" >&2 || true
    exit 1
fi

echo "TEMP1 CALLBACK BIND 25224 MUTANT DETECTED"
