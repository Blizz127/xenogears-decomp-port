#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_TRANSITION_OT_AC99C_BUILD_DIR:-$ROOT/pc_port/build_native/field_transition_ot_ac99c}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

FIELD_SHA="38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc"
FN_SHA="24d83db4df651188f8330a272ae8531f49308ec31b80f3cbdf766a71b9428dd7"
actual="$(sha256sum disc/field.bin | awk '{print $1}')"
if [[ "$actual" != "$FIELD_SHA" ]]; then
    echo "ERROR: disc/field.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
actual="$(dd if=disc/field.bin bs=1 skip=$((0x800AC99C - 0x8006FAF0)) \
    count=$((0x1F4)) status=none | sha256sum | awk '{print $1}')"
if [[ "$actual" != "$FN_SHA" ]]; then
    echo "ERROR: retail func_800AC99C slice SHA-256 mismatch: $actual" >&2
    exit 1
fi

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src -iquote "$ROOT/src/field/main")

build_and_run() {
    local name="$1"
    local linker="$CC"
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" -w "$@" \
        -c "${FIELD_TRANSITION_OT_SOURCE:-src/field/main/misc9.c}" -o "$BUILD_DIR/$name.misc9.o" || return
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/field_transition_ot_ac99c_retail_test.c \
        -o "$BUILD_DIR/$name.test.o" || return
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -fno-pie -no-pie "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.misc9.o" \
        -Wl,--gc-sections -o "$BUILD_DIR/$name" || return
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr" || return
    test ! -s "$BUILD_DIR/$name.stderr"
}

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
rg -q '^FIELD TRANSITION OT AC99C certificate PASS checks=401$' \
    "$BUILD_DIR/O0.stdout"

echo "FIELD TRANSITION OT AC99C O0/O2/UBSAN PASS"

set +e
build_and_run mutant_stale_ot -O0 -g \
    -DFIELD_AC99C_MUTANT_STALE_SETUP_OT
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION transition.ot ' \
       "$BUILD_DIR/mutant_stale_ot.stderr"; then
    echo "STALE-SETUP-OT MUTANT NOT DETECTED rc=$mutant_rc" >&2
    sed -n '1,40p' "$BUILD_DIR/mutant_stale_ot.stderr" >&2 || true
    exit 1
fi

echo "FIELD TRANSITION OT AC99C MUTANT DETECTED"

# The old selected-entry test did not inspect the +92 phase destination.
# Require the full-row certificate to reject a missing third-strip write.
sed '/\*(u16\*)(phaseBase + 0x92) = phase;/d' \
    src/field/main/misc9.c > "$BUILD_DIR/missing_phase.c"
! cmp -s src/field/main/misc9.c "$BUILD_DIR/missing_phase.c"
set +e
FIELD_TRANSITION_OT_SOURCE="$BUILD_DIR/missing_phase.c" \
    build_and_run mutant_missing_phase -O0 -g
mutant_rc=$?
set -e
if [[ "$mutant_rc" -eq 0 ]] || \
   ! rg -q '^ASSERTION transition.ot field=all.phase ' "$BUILD_DIR/mutant_missing_phase.stderr"; then
    echo "MISSING-PHASE MUTANT NOT DETECTED rc=$mutant_rc" >&2
    exit 1
fi
echo "FIELD TRANSITION OT AC99C MISSING-PHASE MUTANT DETECTED"
