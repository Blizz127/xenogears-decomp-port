#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${TEMP1_ANIM_DISPATCH_BUILD_DIR:-$ROOT/pc_port/build_native/temp1_anim_state_dispatch_24730}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
actual="$(sha256sum disc/SLUS_006.64 | awk '{print $1}')"
if [[ "$actual" != "$SLUS_SHA" ]]; then
    echo "ERROR: disc/SLUS_006.64 SHA-256 mismatch: $actual" >&2
    exit 1
fi
FN_SHA="$(dd if=disc/SLUS_006.64 bs=1 \
    skip=$((0x80024730 - 0x8000F800)) count=$((0x1A4)) status=none \
    | sha256sum | awk '{print $1}')"
JTBL_SHA="$(dd if=disc/SLUS_006.64 bs=1 \
    skip=$((0x800186A4 - 0x8000F800)) count=$((0x3C)) status=none \
    | sha256sum | awk '{print $1}')"
if [[ "$FN_SHA" != "e22425f5d12b1260481993beac518a3e6b3dca10b21cf51ab2e2360c92937bec" ||
      "$JTBL_SHA" != "ffe5f84654a036e6c1119c9bdc5ad885e44837fef30040c49df6fb15c36fbe60" ]]; then
    echo "ERROR: retail slice hash mismatch: fn=$FN_SHA jtbl=$JTBL_SHA" >&2
    exit 1
fi
echo "retail func_80024730 slice sha256: $FN_SHA"
echo "retail jtbl_800186A4 slice sha256: $JTBL_SHA"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

# func_80025224 is the only callee the body reaches that also lives in this TU;
# the test supplies the strong stub.
WEAKEN=(--weaken-symbol=func_80025224)

build_and_run() {
    local name="$1"
    local source="$2"
    local linker="$CC"
    shift 2
    # -fno-ipa-ra: never let GCC use the TU-local (weakened) callee body to
    # assume caller-saved registers survive the substituted stub.
    "$CC" "${BASE[@]}" "${INC[@]}" -w -fno-inline -fno-builtin -fno-ipa-ra \
        -fno-sanitize=object-size "$@" \
        -c "$source" -o "$BUILD_DIR/$name.temp1.o"
    objcopy "${WEAKEN[@]}" "$BUILD_DIR/$name.temp1.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/temp1_anim_state_dispatch_24730_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/src/battle_mips_adapter.c \
        -o "$BUILD_DIR/$name.cpu.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -no-pie "$@" "$BUILD_DIR/$name.test.o" \
        "$BUILD_DIR/$name.cpu.o" "$BUILD_DIR/$name.temp1.o" \
        -Wl,--gc-sections -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

build_and_run O0 src/slus_006.64/system/temp1.c -O0 -g
build_and_run O2 src/slus_006.64/system/temp1.c -O2
build_and_run UBSan src/slus_006.64/system/temp1.c -O1 -g \
    -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
rg -q '^ANIM DISPATCH 24730 PASS [0-9]+ retail-oracle comparisons' \
    "$BUILD_DIR/O0.stdout"

echo "TEMP1 ANIM STATE DISPATCH 24730 O0/O2/UBSAN PASS"

run_mutant() {
    local name="$1"
    local expression="$2"
    sed "/^void func_80024730(/,/^}/ { $expression; }" \
        src/slus_006.64/system/temp1.c > "$BUILD_DIR/$name.c"
    set +e
    build_and_run "$name" "$BUILD_DIR/$name.c" -O0 -g
    local rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! rg -q '^ANIM DISPATCH FAIL' "$BUILD_DIR/$name.stderr"; then
        echo "MUTANT NOT DETECTED: $name rc=$rc" >&2
        sed -n '1,40p' "$BUILD_DIR/$name.stderr" >&2 || true
        exit 1
    fi
    echo "  mutant rejected: $name"
}

run_mutant sprite_byte_69  's/pState + 0x2B) = 0x68/pState + 0x2B) = 0x69/'
run_mutant hold_4         's/(pState + 0x36) = 3/(pState + 0x36) = 4/'
run_mutant wrong_table    's/D_8006F99C\[0\]/D_8006F9AC[0]/'
run_mutant mode_step3     's/- 2) & 0xF/- 3) \& 0xF/'
run_mutant tail_mode0     's/func_80025224(pInner, mode)/func_80025224(pInner, 0)/'
run_mutant wrong_callback 's/func_80022E8C);/func_80022DF4);/'

echo 'TEMP1 ANIM STATE DISPATCH 24730 mutants rejected: sprite_byte_69 hold_4 wrong_table mode_step3 tail_mode0 wrong_callback'
