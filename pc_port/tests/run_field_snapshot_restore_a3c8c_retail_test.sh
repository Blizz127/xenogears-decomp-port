#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_SNAPSHOT_RESTORE_A3C8C_BUILD_DIR:-$ROOT/pc_port/build_native/field_snapshot_restore_a3c8c}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

FIELD_SHA="38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"
FN_SHA="38a9a8bf3712ba1d15e8164d53fbeea9c8c90b8c8c74ed7860220ac71d68b450"
actual="$(sha256sum disc/field.bin | awk '{print $1}')"
if [[ "$actual" != "$FIELD_SHA" ]]; then
    echo "ERROR: disc/field.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/field.bin bs=1 skip=$((0x800A3C8C - 0x8006FAF0)) \
    count=$((0x2C0)) status=none | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$FN_SHA" ]]; then
    echo "ERROR: retail func_800A3C8C slice SHA-256 mismatch: $actual" >&2
    exit 1
fi

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
        -c src/field/scripts/virtual_machine.c -o "$BUILD_DIR/$name.vm.o"
    objcopy --weaken-symbol=func_80021D50 \
            "$BUILD_DIR/$name.vm.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror -Wno-array-bounds "$@" \
        -c pc_port/tests/field_snapshot_restore_a3c8c_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -fno-pie -no-pie "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.vm.o" \
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
rg -q '^FIELD SNAPSHOT RESTORE A3C8C certificate PASS checks=16$' \
    "$BUILD_DIR/O0.stdout"

echo "FIELD SNAPSHOT RESTORE A3C8C O0/O2/UBSAN PASS"

set +e
build_and_run mutant_skip_anim -O0 -g \
    -DFIELD_A3C8C_MUTANT_SKIP_ANIM_PATCH
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION snap.restore ' \
       "$BUILD_DIR/mutant_skip_anim.stderr"; then
    echo "SKIP-ANIM MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_skip_anim.stderr" >&2 || true
    exit 1
fi

echo "FIELD SNAPSHOT RESTORE A3C8C MUTANT DETECTED"
