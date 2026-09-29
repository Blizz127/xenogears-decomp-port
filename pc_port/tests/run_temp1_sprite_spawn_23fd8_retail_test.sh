#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${TEMP1_SPRITE_SPAWN_BUILD_DIR:-$ROOT/pc_port/build_native/temp1_sprite_spawn_23fd8}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

SLUS_SHA="dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119"
FN_SHA="$(dd if=disc/SLUS_006.64 bs=1 \
    skip=$((0x80023FD8 - 0x8000F800)) count=$((700)) status=none \
    | sha256sum | awk '{print $1}')"
actual="$(sha256sum disc/SLUS_006.64 | awk '{print $1}')"
if [[ "$actual" != "$SLUS_SHA" ]]; then
    echo "ERROR: disc/SLUS_006.64 SHA-256 mismatch: $actual" >&2
    exit 1
fi
# Same slice already recorded next to the port fallback body.
if [[ "$FN_SHA" != "6291763a42ce40bf3073a031287166b8401aa0e6655b0cd4a47cfad702393c65" ]]; then
    echo "ERROR: retail func_80023FD8 slice SHA-256 mismatch: $FN_SHA" >&2
    exit 1
fi
echo "retail func_80023FD8 slice sha256: $FN_SHA"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

# The five callees the retail body reaches are replaced by the test's
# controlled stubs; the unit under test is the shipped temp1.c body.
WEAKEN=(--weaken-symbol=func_80023440 --weaken-symbol=func_80023468
        --weaken-symbol=func_80023A48 --weaken-symbol=func_80023538
        --weaken-symbol=func_80024730)

build_and_run() {
    local name="$1"
    local source="$2"
    local linker="$CC"
    shift 2
    # -fno-ipa-ra: GCC must not use the TU-local (weakened) bodies to assume
    # those calls leave caller-saved registers intact - the strong test stubs
    # that replace them do clobber them.
    "$CC" "${BASE[@]}" "${INC[@]}" -w -fno-inline -fno-builtin -fno-ipa-ra \
        -fno-sanitize=object-size "$@" \
        -c "$source" -o "$BUILD_DIR/$name.temp1.o"
    objcopy "${WEAKEN[@]}" "$BUILD_DIR/$name.temp1.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "$@" \
        -c pc_port/tests/temp1_sprite_spawn_23fd8_retail_test.c \
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
rg -q '^SPRITE SPAWN 23FD8 PASS [0-9]+ retail-oracle comparisons' \
    "$BUILD_DIR/O0.stdout"

echo "TEMP1 SPRITE SPAWN 23FD8 O0/O2/UBSAN PASS"

run_mutant() {
    local name="$1"
    local expression="$2"
    sed "/^u8\* func_80023FD8(/,/^}/ { $expression; }" \
        src/slus_006.64/system/temp1.c > "$BUILD_DIR/$name.c"
    set +e
    build_and_run "$name" "$BUILD_DIR/$name.c" -O0 -g
    local rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! rg -q '^SPRITE SPAWN FAIL' "$BUILD_DIR/$name.stderr"; then
        echo "MUTANT NOT DETECTED: $name rc=$rc" >&2
        sed -n '1,40p' "$BUILD_DIR/$name.stderr" >&2 || true
        exit 1
    fi
    echo "  mutant rejected: $name"
}

run_mutant entry_plus2    's/index \* 2 + 2/index * 2 + 4/'
run_mutant gate_eq1       's/D_800591AD != 0/D_800591AD == 1/'
run_mutant inherit_1c     's/pParent + 0x18/pParent + 0x1C/'
run_mutant split_shift1   's/split >> 2/split >> 1/'
run_mutant duration_zero  's/duration = (u16)D_800591A8/duration = (u16)0/'
run_mutant tag_1000       's/0x20000000/0x10000000/'
run_mutant a1_zero        's/(s32)(uintptr_t)pAnimData/(s32)0/'
run_mutant zero_to_one    's/= 0;/= 1;/'

echo 'TEMP1 SPRITE SPAWN 23FD8 mutants rejected: entry_plus2 gate_eq1 inherit_1c split_shift1 duration_zero tag_1000 a1_zero zero_to_one'
