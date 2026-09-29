#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${BATTLE_LEAF_BF6CC_BUILD_DIR:-$ROOT/pc_port/build_native/battle_leaf_setters_bf6cc}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BATTLE_SHA="1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291"
actual="$(sha256sum disc/battle.bin | awk '{print $1}')"
if [[ "$actual" != "$BATTLE_SHA" ]]; then
    echo "ERROR: disc/battle.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
echo "retail func_800BF6CC slice sha256: $(dd if=disc/battle.bin bs=1 \
    skip=$((0x800BF6CC - 0x8006FAF0)) count=$((0x2C)) status=none | sha256sum | awk '{print $1}')"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

mapfile -t TUS < <(ls src/battle/main*.c)

build_and_run() {
    local name="$1"
    shift
    local linker="$CC"
    # The landed bodies live in three TUs (one per asm run + C run pair); the
    # arguments are the TU sources (…​.c) followed by the cc flags (-…).
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
        -c pc_port/tests/battle_leaf_setters_bf6cc_retail_test.c \
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
rg -q '^BATTLE LEAF BF6CC PASS checks=[0-9]+' "$BUILD_DIR/O0.stdout"

echo "BATTLE LEAF BF6CC O0/O2/UBSAN PASS"

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

run_mutant gate_eq1 func_800BF6CC 's/D_800C3610 != 0/D_800C3610 == 1/'
run_mutant sub_swapped func_800BF6F8 's/return D_80059464 - marker;/return marker - D_80059464;/'
run_mutant bool_inverted func_800BF720 's/return D_800D2D68 != 0;/return D_800D2D68 == 0;/'
run_mutant invert_mask func_80089C24 's/return 0xFFFF \^ (u32)D_800C3468\[index\];/return 0xFFFE ^ (u32)D_800C3468[index];/'
run_mutant range_17 func_80089C6C 's/if (index >= 0x10) {/if (index >= 0x11) {/'
run_mutant entry_1 func_8008AB4C 's/ArchiveSetIndex(0x20, 0);/ArchiveSetIndex(0x20, 1);/'

echo 'BATTLE LEAF BF6CC mutants rejected: gate_eq1 sub_swapped bool_inverted invert_mask range_17 entry_1'
