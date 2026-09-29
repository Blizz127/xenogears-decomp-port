#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_WALKMESH_D3D4_BUILD_DIR:-$ROOT/pc_port/build_native/field_walkmesh_d3d4_dispatch}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"
# /tmp here is a 16G tmpfs with a per-user quota; keep cc1 temporaries out of it.
export TMPDIR="${TMPDIR:-/var/tmp}"

FIELD_SHA="38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"
D3D4_SHA="f0f4f0d2bc03b974e05e7393db59ba4414c72b2c01e7f5225c83bf6824fa9a8a"
actual="$(sha256sum disc/field.bin | awk '{print $1}')"
if [[ "$actual" != "$FIELD_SHA" ]]; then
    echo "ERROR: disc/field.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/field.bin bs=1 skip=$((0x8007D3D4 - 0x8006FAF0)) \
    count=$((0x8007D818 - 0x8007D3D4)) status=none | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$D3D4_SHA" ]]; then
    echo "ERROR: retail func_8007D3D4 slice SHA-256 mismatch: $actual" >&2
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
    "$CC" "${BASE[@]}" "${INC[@]}" -w "$@" \
        -c src/field/main/misc4.c -o "$BUILD_DIR/$name.misc4.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/field_walkmesh_d3d4_dispatch_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        # This image's GCC linker script names a missing libubsan.so.1.0.0;
        # Clang supplies a complete compatible standalone runtime.  Keep GCC
        # as the compiler so the legacy production TU uses the native flags.
        linker=clang
    fi
    "$linker" -fno-pie -no-pie "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.misc4.o" \
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
rg -q '^FIELD WALKMESH D3D4 DISPATCH certificate PASS checks=33$' \
    "$BUILD_DIR/O0.stdout"

echo "FIELD WALKMESH D3D4 DISPATCH O0/O2/UBSAN PASS"

set +e
build_and_run swap12 -O0 -g \
    -DFIELD_WALKMESH_MUTANT_D3D4_SWAP12
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION d3d4.dispatch.result ' \
       "$BUILD_DIR/swap12.stderr"; then
    echo "D3D4 SWAP12 MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/swap12.stderr" >&2 || true
    exit 1
fi

echo "FIELD WALKMESH D3D4 DISPATCH MUTANT DETECTED"
