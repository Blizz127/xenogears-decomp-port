#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${BATTLE_BOARD_FAMILY_BUILD_DIR:-$ROOT/pc_port/build_native/battle_board_family_7e8}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BATTLE_SHA="1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291"
actual="$(sha256sum disc/battle.bin | awk '{print $1}')"
if [[ "$actual" != "$BATTLE_SHA" ]]; then
    echo "ERROR: disc/battle.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
echo "retail 0x8007E934..0x8007EF6C slice sha256: $(dd if=disc/battle.bin bs=1 \
    skip=$((0x8007E934 - 0x8006FAF0)) count=$((0x638)) status=none | sha256sum | awk '{print $1}')"
E780_SHA="$(dd if=disc/battle.bin bs=1 \
    skip=$((0x8007E780 - 0x8006FAF0)) count=$((0x40)) status=none | sha256sum | awk '{print $1}')"
if [[ "$E780_SHA" != "701e86f7db26624e4386aaa54826a84da53c9aebff392e8a8c714db039e4ad55" ]]; then
    echo "ERROR: retail func_8007E780 slice SHA-256 mismatch: $E780_SHA" >&2
    exit 1
fi
echo "retail 0x8007E780 slice sha256: $E780_SHA"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

# every TU that carries a landed body of the family
mapfile -t TUS < <(ls src/battle/main*.c)

build_and_run() {
    local name="$1"
    shift
    local linker="$CC"
    local sources=() flags=() objs=() a o
    for a in "$@"; do
        case "$a" in
            *.c) sources+=("$a") ;;
            *) flags+=("$a") ;;
        esac
    done
    for a in "${sources[@]}"; do
        o="$BUILD_DIR/$name.$(basename "$a").o"
        "$CC" "${BASE[@]}" "${INC[@]}" -w -fno-inline -fno-builtin -fno-ipa-ra \
            -fno-sanitize=object-size "${flags[@]}" -c "$a" -o "$o"
        objs+=("$o")
    done
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/tests/battle_board_family_7e8_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -no-pie "${flags[@]}" "$BUILD_DIR/$name.test.o" \
        "$BUILD_DIR/$name.cpu.o" "${objs[@]}" -Wl,--gc-sections \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

build_and_run O0 "${TUS[@]}" -O0 -g
build_and_run O2 "${TUS[@]}" -O2
build_and_run UBSan "${TUS[@]}" -O1 -g \
    -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
rg -q '^BOARD FAMILY 7E8 PASS checks=[0-9]+' "$BUILD_DIR/O0.stdout"

echo "BATTLE BOARD FAMILY 7E8 O0/O2/UBSAN PASS"

run_mutant() {
    local name="$1" fn="$2" expression="$3"
    local target sources=() s
    target="$(rg -l --glob 'src/battle/main*.c' "/\* ${fn}\\.s" src/battle 2>/dev/null | head -1)"
    if [[ -z "$target" ]]; then
        echo "MUTANT TARGET NOT FOUND: $name ($fn)" >&2
        exit 1
    fi
    sed "$expression" "$target" > "$BUILD_DIR/$name.c"
    if cmp -s "$target" "$BUILD_DIR/$name.c"; then
        echo "MUTANT PATTERN DID NOT APPLY: $name" >&2
        exit 1
    fi
    for s in "${TUS[@]}"; do
        if [ "$s" = "$target" ]; then
            sources+=("$BUILD_DIR/$name.c")
        else
            sources+=("$s")
        fi
    done
    set +e
    build_and_run "$name" "${sources[@]}" -O0 -g
    local rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! rg -q '^(BOARD FAMILY|BATTLE LEAF) FAIL' "$BUILD_DIR/$name.stderr"; then
        echo "MUTANT NOT DETECTED: $name rc=$rc" >&2
        sed -n '1,20p' "$BUILD_DIR/$name.stderr" >&2 || true
        exit 1
    fi
    echo "  mutant rejected: $name"
}

run_mutant quotient_to_product func_8007E780 's/pRow\[p\[1\]\] \/= p\[2\];/pRow[p[1]] *= p[2];/'
run_mutant copy_swapped func_8007B958 's/pRow\[p\[2\]\] = pRow\[p\[1\]\];/pRow[p[1]] = pRow[p[2]];/'
run_mutant scan_stride_29 func_8007EEE8 's/i \* 0x1C/i * 0x1D/'
run_mutant table_slot_swap func_8007B914 's/D_8005A3A0\[i2\] = /D_8005A3A0[i1] = /'
run_mutant row_xor_to_and func_8007E98C 's/\^ (i2 | i3s)) == 0;/\& (i2 | i3s)) == 0;/'
run_mutant ac_off_shift func_8007E8AC 's/0x140/0x141/'
run_mutant xor_to_and func_8007EAC8 's/pRow\[p\[1\]\] \^ pRow\[p\[2\]\]/pRow[p[1]] \& pRow[p[2]]/'
run_mutant ge_to_gt func_8007EB50 's/return b >= a;/return b > a;/'
run_mutant mask_to_or func_8007EBD8 's/return (a \& b) != 0;/return (a | b) != 0;/'
run_mutant shift_8 func_8007EF44 's/>> 7;/>> 8;/'
run_mutant index_plus4 func_8007EF44 's/(index + 3)/(index + 4)/'

echo 'BATTLE BOARD FAMILY 7E8 mutants rejected: quotient_to_product scan_stride_29 table_slot_swap row_xor_to_and ac_off_shift copy_swapped xor_to_and ge_to_gt mask_to_or shift_8 index_plus4'
