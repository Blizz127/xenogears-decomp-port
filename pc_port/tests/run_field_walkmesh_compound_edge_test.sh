#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_WALKMESH_EDGE_BUILD_DIR:-$ROOT/pc_port/build_native/field_walkmesh_edge}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

FIELD_SHA="38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"
BEF4_SHA="6afec5d69df2e6ac7501e6c9f38f632354789a051e27ab083e9c858bb174941b"
actual="$(sha256sum disc/field.bin | awk '{print $1}')"
if [[ "$actual" != "$FIELD_SHA" ]]; then
    echo "ERROR: disc/field.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/field.bin bs=1 skip=$((0x8007BEF4 - 0x8006FAF0)) \
    count=$((0x8007C670 - 0x8007BEF4)) status=none | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$BEF4_SHA" ]]; then
    echo "ERROR: retail func_8007BEF4 slice SHA-256 mismatch: $actual" >&2
    exit 1
fi

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

build_and_run() {
    local name="$1"
    local linker="$CC"
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" -w -fpermissive "$@" \
        -c "${FIELD_WALKMESH_EDGE_SOURCE:-src/field/main/misc4.c}" -o "$BUILD_DIR/$name.misc4.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/field_walkmesh_compound_edge_test.c \
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
grep -q '^FIELD WALKMESH COMPOUND EDGE certificate PASS checks=54$' \
    "$BUILD_DIR/O0.stdout"

echo "FIELD WALKMESH COMPOUND EDGE O0/O2/UBSAN PASS"

set +e
build_and_run old_compound_mask -O0 -g \
    -DFIELD_WALKMESH_MUTANT_KEEP_COMPOUND_SIDE
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! grep -q '^ASSERTION compound.side.normalized ' \
       "$BUILD_DIR/old_compound_mask.stderr"; then
    echo "OLD COMPOUND MASK MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/old_compound_mask.stderr" >&2 || true
    exit 1
fi

echo "FIELD WALKMESH COMPOUND EDGE MUTANT DETECTED"

# Remove only the native shared-tail repair: the corrected expectations must
# reject the original field-12 defect, independently of compound-side logic.
python3 - "$BUILD_DIR/missing_tail.c" <<'PYMUTANT'
from pathlib import Path
import sys
source = Path("src/field/main/misc4.c").read_text()
block = """#ifdef XENO_PC_PORT
            /* 8007C4C0 / 8007C570 jump to the b.z store at 8007C634. */
            ((u16*)outEdge)[0x6] = (u16)b[2];
#endif"""
assert source.count(block) == 2
Path(sys.argv[1]).write_text(source.replace(block, ""))
PYMUTANT
set +e
FIELD_WALKMESH_EDGE_SOURCE="$BUILD_DIR/missing_tail.c" build_and_run missing_tail -O0 -g
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || ! grep -q 'field=b.z actual=27499 expected=22' "$BUILD_DIR/missing_tail.stderr"; then
    echo "MISSING SHARED TAIL MUTANT NOT DETECTED rc=$mutant_rc" >&2
    exit 1
fi
echo "FIELD WALKMESH SHARED TAIL MUTANT DETECTED"
