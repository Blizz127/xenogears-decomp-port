#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_OPCODE_98430_BUILD_DIR:-$ROOT/pc_port/build_native/field_opcode_98430}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

FIELD_SHA="38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"
OP_SHA="9dfa2e84c6c815376e80524ba06bb4a60348b0307f09988f566308bdf97c47bd"
actual="$(sha256sum disc/field.bin | awk '{print $1}')"
if [[ "$actual" != "$FIELD_SHA" ]]; then
    echo "ERROR: disc/field.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/field.bin bs=1 skip=$((0x80098430 - 0x8006FAF0)) \
    count=$((0xBC)) status=none | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$OP_SHA" ]]; then
    echo "ERROR: retail func_80098430 slice SHA-256 mismatch: $actual" >&2
    exit 1
fi

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -ffunction-sections -fdata-sections)
# -fpermissive only for the game TU (built with -w): gcc < 14 rejects it for C
# as a warning, which -Werror on the test file would promote to an error.
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

build_and_run() {
    local name="$1"
    local linker="$CC"
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" -fpermissive -w "$@" \
        -c src/field/main/misc7.c -o "$BUILD_DIR/$name.misc7.o"
    objcopy --weaken-symbol=func_80099AC0 "$BUILD_DIR/$name.misc7.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/field_opcode_98430_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -fno-pie -no-pie "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.misc7.o" \
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
rg -q '^FIELD OPCODE 98430 certificate PASS checks=11$' \
    "$BUILD_DIR/O0.stdout"

echo "FIELD OPCODE 98430 O0/O2/UBSAN PASS"

set +e
build_and_run mutant_mask -O0 -g \
    -DFIELD_98430_MUTANT_SKIP_MASK
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION opcode.98430 ' \
       "$BUILD_DIR/mutant_mask.stderr"; then
    echo "SKIP-MASK MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_mask.stderr" >&2 || true
    exit 1
fi

echo "FIELD OPCODE 98430 MUTANT DETECTED"
