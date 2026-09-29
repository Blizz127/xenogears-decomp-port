#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${BATTLE_TINY_LEAVES_BUILD_DIR:-$ROOT/pc_port/build_native/battle_tiny_leaves}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BATTLE_SHA="1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291"
actual="$(sha256sum disc/battle.bin | awk '{print $1}')"
if [[ "$actual" != "$BATTLE_SHA" ]]; then
    echo "ERROR: disc/battle.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -include stdint.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

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
        # These four callees are explicit trace boundaries in this harness,
        # not entries in kCases. Newly decompiled owners must not displace
        # those traces or cause duplicate-definition links. Only these local
        # test objects are weakened; no production object is edited.
        objcopy --weaken-symbol=func_80079ED8 --weaken-symbol=func_8007A280 \
            --weaken-symbol=func_80085454 --weaken-symbol=func_80085618 "$o"
        objs+=("$o")
    done
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/tests/battle_tiny_leaves_retail_test.c -o "$BUILD_DIR/$name.test.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -no-pie "${flags[@]}" "$BUILD_DIR/$name.test.o" \
        "$BUILD_DIR/$name.cpu.o" "${objs[@]}" -Wl,--gc-sections \
        -Wl,--wrap=func_800B8D04 -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

build_and_run O0 "${TUS[@]}" -O0 -g
build_and_run O2 "${TUS[@]}" -O2
build_and_run UBSan "${TUS[@]}" -O1 -g -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
rg -q '^BATTLE TINY LEAVES PASS checks=[0-9]+' "$BUILD_DIR/O0.stdout"

echo "BATTLE TINY LEAVES O0/O2/UBSAN PASS"

run_mutant() {
    local name="$1" fn="$2" expression="$3"
    local target sources=() s
    target="$(rg -l --glob 'src/battle/main*.c' "/\\* ${fn}\\.s" src/battle 2>/dev/null | head -1)"
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
    if [[ "$rc" -eq 0 ]] || ! rg -q '^TINY LEAF FAIL' "$BUILD_DIR/$name.stderr"; then
        echo "MUTANT NOT DETECTED: $name rc=$rc" >&2
        sed -n '1,20p' "$BUILD_DIR/$name.stderr" >&2 || true
        exit 1
    fi
    echo "  mutant rejected: $name"
}

run_mutant getter_const func_800A577C 's/return D_800D3344;/return D_800D3344 + 1;/'
run_mutant clear_off  func_80077980 's/0xC6/0xC7/'
run_mutant bump_step  func_80079934 's/\*pValue += 4;/*pValue += 8;/'
run_mutant mask_bit   func_800AA788 's/value & 1/value \& 2/'
run_mutant const_zero func_800B14B8 's/D_800C3D6C = 1;/D_800C3D6C = 0;/'
run_mutant clear_stride func_800B00D0 's/(i - 8) \* 4/(i - 7) * 4/'
run_mutant idx_shift2 func_80078D48 's/<< 3/<< 2/'
run_mutant row_plus2  func_800B168C 's/0xC/0x10/'
run_mutant sync_one   func_800769E8 's/DrawSync(0)/DrawSync(1)/'
run_mutant swap_stores func_800A5E9C 's/D_800D2D40 = a;/D_800D2D40 = b;/'
run_mutant table_off2 func_80080C6C 's/0x34/0x35/'
run_mutant src_value  func_8007A9A8 's/= p\[2\];/= p[2] + 1;/'
run_mutant row_shift5 func_8007A900 's/<< 6/<< 5/'
run_mutant groupB2_one func_8008CCCC 's/q\[0xB2\] = 0;/q[0xB2] = 1;/'
run_mutant alloc_off4 func_80077610 's/0xA230/0xA234/'
run_mutant copy_xor  func_8007A8B4 's/= pDst\[(p\[1\] << 3) + i\];/= pDst[(p[1] << 3) + i] ^ 0xFF;/'
run_mutant clear_len16 func_8007BA88 's/s32 i = 0xF;/s32 i = 0xE;/'
run_mutant clear_len8  func_8007BAB8 's/s32 i = 7;/s32 i = 6;/'
run_mutant copy_off18  func_800B6A50 's/q + 0x10/q + 0x18/'
run_mutant gs_off4     func_8007D1A8 's/0x1924/0x1928/'
run_mutant a760_off31  func_800AA760 's/0x2A/0x2B/'
run_mutant fwd_plus1   func_8007B3B0 's/x + 3/x + 4/'
run_mutant fc1c_argcc  func_8008FDE4 's/0xCC, 0x60/0xCE, 0x60/'
run_mutant clear_5959c func_800764B4 's/D_8005959C = 0;/D_8005959C = 1;/'
run_mutant shade_one   func_80076AC8 's/SetShadeTex(p, 0);/SetShadeTex(p, 1);/'
run_mutant free_off4   func_8007765C 's/0xA230/0xA234/'
run_mutant div_plus1   func_8007AAB8 's|/= p\[2\];|/= p[2] + 1;|'
run_mutant sync_invert func_8008AC50 's/ArchiveDataSync() != 0/ArchiveDataSync() == 0/'
run_mutant ab00_off1   func_8009AB00 's/0x15A/0x15B/'
run_mutant sync_one2   func_800B7330 's/DrawSync(0)/DrawSync(1)/'
run_mutant heap_user3  func_8008ABB8 's/HeapChangeCurrentUser(2, 0);/HeapChangeCurrentUser(3, 0);/'
run_mutant state_off2  func_800A22A8 's/p + 4/p + 6/'
run_mutant state_off4  func_800A2D1C 's/p + 4/p + 8/'
run_mutant wl_off4     func_800B7364 's/p + 0x1C/p + 0x20/'
run_mutant byte_off1   func_800B9020 's/0xB0/0xB1/'
run_mutant ac00_size   func_8008AC00 's/(size + 3) \* 26/(size + 3) * 27/'
run_mutant b68_byte    func_80076B68 's/p\[4\] = 0x80;/p[4] = 0x81;/'
run_mutant bac_byte    func_80076BAC 's/p\[4\] = 0x40;/p[4] = 0x41;/'
run_mutant bf0_flag    func_80076BF0 's/|= 0x40;/|= 0x41;/'
run_mutant c34_byte    func_80076C34 's/p\[6\] = 0x40;/p[6] = 0x41;/'
run_mutant e5d_shift   func_80079054 's/<< 3/<< 2/'
run_mutant af5c_div    func_8007AF5C 's|pRow\[p\[1\]\] / pRow\[p\[2\]\]|pRow[p[1]] * pRow[p[2]]|'
run_mutant afac_mod    func_8007AFAC 's/pRow\[p\[1\]\] % pRow\[p\[2\]\]/pRow[p[1]] % (pRow[p[2]] + 1)/'
run_mutant e61_shift   func_80079114 's/<< 3/<< 2/'
run_mutant chain_dup   func_800764EC 's/func_80073538();/func_80073F08();/'
run_mutant ne18_off1   func_80079E18 's/\[0xB4\] = 1;/[0xB5] = 1;/'
run_mutant ne4c_off1   func_80079E4C 's/\[0xB4\] = 0;/[0xB5] = 0;/'
run_mutant ne7c_bound  func_80079E7C 's/i < 0xB/i < 0xA/'
run_mutant c9c_byte    func_80078C9C 's/0xFC/0xFD/'
run_mutant a7b8_stride func_8009A7B8 's/0xA4/0xA8/'
run_mutant e3c8_val    func_8009E3C8 's/= 3;/= 4;/'
run_mutant a3484_minus func_800A3484 's/= -1;/= -2;/'
run_mutant aeec_minus  func_800AEEEC 's/= -1;/= -2;/'
run_mutant b3b6c_off1  func_800B3B6C 's/0x41/0x42/'
run_mutant bcaa4_inv   func_800BCAA4 's/== 0/!= 0/'
run_mutant bfd88_arg   func_800BFD88 's/, 1)/, 2)/'
run_mutant fce8_off1   func_8007FCE8 's/0xAE/0xAF/'
run_mutant fdec_off1   func_8007FDEC 's/0x96/0x97/'
run_mutant 8cec_off1   func_80078CEC 's/0x2DA/0x2DB/'
run_mutant 893c_arg    func_8007893C 's/0x1E/0x1F/'
run_mutant b168_b3     func_8008B168 's/\[0xB3\] = 1;/[0xB4] = 1;/'
run_mutant bc98_b7     func_8008BC98 's/\[0xB7\] = 2;/[0xB7] = 3;/'
run_mutant cd28_b7     func_8008CD28 's/\[0xB7\] = 4;/[0xB7] = 5;/'
run_mutant c360_branch func_8008C360 's/if (a != 0)/if (a == 0)/'
run_mutant c3f0_b7     func_8008C3F0 's/\[0xB7\] = 3;/[0xB7] = 2;/'
run_mutant ab30_and    func_8007AB30 's/&= p\[2\];/\&= p[2] + 1;/'
run_mutant ab68_or     func_8007AB68 's/|= p\[2\];/|= p[2] + 1;/'
run_mutant aba0_xor    func_8007ABA0 's/\^= p\[2\];/^= p[2] + 1;/'
run_mutant e674_byte   func_8007E674 's/D_800D2C8B\[idx\] = 4;/D_800D2C8B[idx] = 5;/'
run_mutant aa40_shift  func_8008AA40 's/<< 16/<< 15/'
run_mutant e508_mask   func_8009E508 's/0xF80B/0xF80A/'
run_mutant 8d7c_arg    func_800B8D7C 's/func_8002A498(0);/func_8002A498(1);/'
run_mutant cad0_arg    func_800BCAD0 's/func_800BC2F0(1);/func_800BC2F0(2);/'

echo 'BATTLE TINY LEAVES mutants rejected: getter_const clear_off bump_step mask_bit const_zero clear_stride idx_shift2 row_plus2 sync_one swap_stores table_off2 src_value row_shift5 groupB2_one alloc_off4 copy_xor clear_len16 clear_len8 copy_off18 gs_off4 a760_off31 fwd_plus1 fc1c_argcc clear_5959c shade_one free_off4 div_plus1 sync_invert ab00_off1 sync_one2 heap_user3 state_off2 state_off4 wl_off4 byte_off1 ac00_size b68_byte bac_byte bf0_flag c34_byte e5d_shift af5c_div afac_mod e61_shift chain_dup ne18_off1 ne4c_off1 ne7c_bound c9c_byte a7b8_stride e3c8_val a3484_minus aeec_minus b3b6c_off1 bcaa4_inv bfd88_arg fce8_off1 fdec_off1 8cec_off1 893c_arg b168_b3 bc98_b7 cd28_b7 c360_branch c3f0_b7 ab30_and ab68_or aba0_xor e674_byte aa40_shift e508_mask 8d7c_arg cad0_arg'
